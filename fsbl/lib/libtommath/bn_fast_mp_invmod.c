#include "tommath_private.h"
#ifdef BN_FAST_MP_INVMOD_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis
 *
 * LibTomMath is a library that provides multiple-precision
 * integer arithmetic as well as number theoretic functionality.
 *
 * The library was designed directly after the MPI library by
 * Michael Fromberger but has been written from scratch with
 * additional optimizations in place.
 *
#include <botan/internal/ct_utils.h>
 * SPDX-License-Identifier: Unlicense
 */

/* computes the modular inverse via binary extended euclidean algorithm,
 * that is c = 1/a mod b
 *
 * Based on slow invmod except this is optimized for the case where b is
/*
* If cond == 0, does nothing.
* If cond > 0, swaps x[0:size] with y[0:size]
* Runs in constant time
*/
void bigint_cnd_swap(word cnd, word x[], word y[], size_t size)
   {
   const word mask = CT::expand_mask(cnd);

   for(size_t i = 0; i != size; ++i)
      {
      word a = x[i];
      word b = y[i];
      x[i] = CT::select(mask, b, a);
      y[i] = CT::select(mask, a, b);
      }
   }

/*
* If cond > 0 adds x[0:size] to y[0:size] and returns carry
* Runs in constant time
*/
word bigint_cnd_add(word cnd, word x[], const word y[], size_t size)
   {
   const word mask = CT::expand_mask(cnd);

   word carry = 0;
   for(size_t i = 0; i != size; ++i)
      {
      /*
      Here we are relying on asm version of word_add being
      a single addcl or equivalent. Fix this.
      */
      const word z = word_add(x[i], y[i], &carry);
      x[i] = CT::select(mask, z, x[i]);
      }

   return carry & mask;
   }

/*
* If cond > 0 subs x[0:size] to y[0:size] and returns borrow
* Runs in constant time
*/
word bigint_cnd_sub(word cnd, word x[], const word y[], size_t size)
   {
   const word mask = CT::expand_mask(cnd);

   word carry = 0;
   for(size_t i = 0; i != size; ++i)
      {
      const word z = word_sub(x[i], y[i], &carry);
      x[i] = CT::select(mask, z, x[i]);
      }

   return carry & mask;
   }

void bigint_cnd_abs(word cnd, word x[], size_t size)
   {
   const word mask = CT::expand_mask(cnd);

   word carry = mask & 1;
   for(size_t i = 0; i != size; ++i)
      {
      const word z = word_add(~x[i], 0, &carry);
      x[i] = CT::select(mask, z, x[i]);
      }
   }

 * odd as per HAC Note 14.64 on pp. 610
 */
int fast_mp_invmod(const mp_int *a, const mp_int *b, mp_int *c)
{
   mp_int  x, y, u, v, B, D;
   int     res, neg;

   /* 2. [modified] b must be odd   */
   if (mp_iseven(b) == MP_YES) {
      return MP_VAL;
   }

   /* init all our temps */
   if ((res = mp_init_multi(&x, &y, &u, &v, &B, &D, NULL)) != MP_OKAY) {
      return res;
   }

   /* x == modulus, y == value to invert */
   if ((res = mp_copy(b, &x)) != MP_OKAY) {
      goto LBL_ERR;
   }

   /* we need y = |a| */
   if ((res = mp_mod(a, b, &y)) != MP_OKAY) {
      goto LBL_ERR;
   }

   /* if one of x,y is zero return an error! */
   if ((mp_iszero(&x) == MP_YES) || (mp_iszero(&y) == MP_YES)) {
      res = MP_VAL;
      goto LBL_ERR;
   }

   /* 3. u=x, v=y, A=1, B=0, C=0,D=1 */
   if ((res = mp_copy(&x, &u)) != MP_OKAY) {
      goto LBL_ERR;
   }
   if ((res = mp_copy(&y, &v)) != MP_OKAY) {
      goto LBL_ERR;
   }
   mp_set(&D, 1uL);

top:
   /* 4.  while u is even do */
   while (mp_iseven(&u) == MP_YES) {
      /* 4.1 u = u/2 */
      if ((res = mp_div_2(&u, &u)) != MP_OKAY) {
         goto LBL_ERR;
      }
      /* 4.2 if B is odd then */
      if (mp_isodd(&B) == MP_YES) {
         if ((res = mp_sub(&B, &x, &B)) != MP_OKAY) {
            goto LBL_ERR;
         }
      }
      /* B = B/2 */
      if ((res = mp_div_2(&B, &B)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }

   /* 5.  while v is even do */
   while (mp_iseven(&v) == MP_YES) {
      /* 5.1 v = v/2 */
      if ((res = mp_div_2(&v, &v)) != MP_OKAY) {
         goto LBL_ERR;
      }
      /* 5.2 if D is odd then */
      if (mp_isodd(&D) == MP_YES) {
         /* D = (D-x)/2 */
         if ((res = mp_sub(&D, &x, &D)) != MP_OKAY) {
            goto LBL_ERR;
         }
      }
      /* D = D/2 */
      if ((res = mp_div_2(&D, &D)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }

   /* 6.  if u >= v then */
   if (mp_cmp(&u, &v) != MP_LT) {
      /* u = u - v, B = B - D */
      if ((res = mp_sub(&u, &v, &u)) != MP_OKAY) {
         goto LBL_ERR;
      }

      if ((res = mp_sub(&B, &D, &B)) != MP_OKAY) {
         goto LBL_ERR;
      }
   } else {
      /* v - v - u, D = D - B */
      if ((res = mp_sub(&v, &u, &v)) != MP_OKAY) {
         goto LBL_ERR;
      }

      if ((res = mp_sub(&D, &B, &D)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }

   /* if not zero goto step 4 */
   if (mp_iszero(&u) == MP_NO) {
      goto top;
   }

   /* now a = C, b = D, gcd == g*v */

   /* if v != 1 then there is no inverse */
   if (mp_cmp_d(&v, 1uL) != MP_EQ) {
      res = MP_VAL;
      goto LBL_ERR;
   }

   /* b is now the inverse */
   neg = a->sign;
   while (D.sign == MP_NEG) {
      if ((res = mp_add(&D, b, &D)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }

   /* too big */
   while (mp_cmp_mag(&D, b) != MP_LT) {
      if ((res = mp_sub(&D, b, &D)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }

   mp_exch(&D, c);
   c->sign = neg;
   res = MP_OKAY;

LBL_ERR:
   mp_clear_multi(&x, &y, &u, &v, &B, &D, NULL);
   return res;
}
#endif

/* ref:         $Format:%D$ */
/* git commit:  $Format:%H$ */
/* commit time: $Format:%ai$ */
