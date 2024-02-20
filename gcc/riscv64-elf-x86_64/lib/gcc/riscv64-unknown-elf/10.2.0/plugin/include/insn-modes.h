/* Generated automatically from machmode.def and config/riscv/riscv-modes.def
   by genmodes.  */

#ifndef GCC_INSN_MODES_H
#define GCC_INSN_MODES_H

enum machine_mode
{
  E_VOIDmode,              /* machmode.def:189 */
#define HAVE_VOIDmode
#ifdef USE_ENUM_MODES
#define VOIDmode E_VOIDmode
#else
#define VOIDmode ((void) 0, E_VOIDmode)
#endif
  E_BLKmode,               /* machmode.def:193 */
#define HAVE_BLKmode
#ifdef USE_ENUM_MODES
#define BLKmode E_BLKmode
#else
#define BLKmode ((void) 0, E_BLKmode)
#endif
  E_CCmode,                /* machmode.def:231 */
#define HAVE_CCmode
#ifdef USE_ENUM_MODES
#define CCmode E_CCmode
#else
#define CCmode ((void) 0, E_CCmode)
#endif
  E_BImode,                /* machmode.def:196 */
#define HAVE_BImode
#ifdef USE_ENUM_MODES
#define BImode E_BImode
#else
#define BImode (scalar_int_mode ((scalar_int_mode::from_int) E_BImode))
#endif
  E_QImode,                /* machmode.def:204 */
#define HAVE_QImode
#ifdef USE_ENUM_MODES
#define QImode E_QImode
#else
#define QImode (scalar_int_mode ((scalar_int_mode::from_int) E_QImode))
#endif
  E_HImode,                /* machmode.def:205 */
#define HAVE_HImode
#ifdef USE_ENUM_MODES
#define HImode E_HImode
#else
#define HImode (scalar_int_mode ((scalar_int_mode::from_int) E_HImode))
#endif
  E_SImode,                /* machmode.def:206 */
#define HAVE_SImode
#ifdef USE_ENUM_MODES
#define SImode E_SImode
#else
#define SImode (scalar_int_mode ((scalar_int_mode::from_int) E_SImode))
#endif
  E_DImode,                /* machmode.def:207 */
#define HAVE_DImode
#ifdef USE_ENUM_MODES
#define DImode E_DImode
#else
#define DImode (scalar_int_mode ((scalar_int_mode::from_int) E_DImode))
#endif
  E_TImode,                /* machmode.def:208 */
#define HAVE_TImode
#ifdef USE_ENUM_MODES
#define TImode E_TImode
#else
#define TImode (scalar_int_mode ((scalar_int_mode::from_int) E_TImode))
#endif
  E_OImode,                /* config/riscv/riscv-modes.def:44 */
#define HAVE_OImode
#ifdef USE_ENUM_MODES
#define OImode E_OImode
#else
#define OImode (scalar_int_mode ((scalar_int_mode::from_int) E_OImode))
#endif
  E_VNx2x2DImode,          /* config/riscv/riscv-seg-modes.def:3 */
#define HAVE_VNx2x2DImode
#ifdef USE_ENUM_MODES
#define VNx2x2DImode E_VNx2x2DImode
#else
#define VNx2x2DImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x2DImode))
#endif
  E_VNx2x16QImode,         /* config/riscv/riscv-seg-modes.def:7 */
#define HAVE_VNx2x16QImode
#ifdef USE_ENUM_MODES
#define VNx2x16QImode E_VNx2x16QImode
#else
#define VNx2x16QImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x16QImode))
#endif
  E_VNx2x8HFmode,          /* config/riscv/riscv-seg-modes.def:12 */
#define HAVE_VNx2x8HFmode
#ifdef USE_ENUM_MODES
#define VNx2x8HFmode E_VNx2x8HFmode
#else
#define VNx2x8HFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x8HFmode))
#endif
  E_VNx2x4SImode,          /* config/riscv/riscv-seg-modes.def:17 */
#define HAVE_VNx2x4SImode
#ifdef USE_ENUM_MODES
#define VNx2x4SImode E_VNx2x4SImode
#else
#define VNx2x4SImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x4SImode))
#endif
  E_VNx2x8HImode,          /* config/riscv/riscv-seg-modes.def:25 */
#define HAVE_VNx2x8HImode
#ifdef USE_ENUM_MODES
#define VNx2x8HImode E_VNx2x8HImode
#else
#define VNx2x8HImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x8HImode))
#endif
  E_VNx2x4SFmode,          /* config/riscv/riscv-seg-modes.def:44 */
#define HAVE_VNx2x4SFmode
#ifdef USE_ENUM_MODES
#define VNx2x4SFmode E_VNx2x4SFmode
#else
#define VNx2x4SFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x4SFmode))
#endif
  E_VNx2x2DFmode,          /* config/riscv/riscv-seg-modes.def:49 */
#define HAVE_VNx2x2DFmode
#ifdef USE_ENUM_MODES
#define VNx2x2DFmode E_VNx2x2DFmode
#else
#define VNx2x2DFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x2DFmode))
#endif
  E_VNx3x8HFmode,          /* config/riscv/riscv-seg-modes.def:26 */
#define HAVE_VNx3x8HFmode
#ifdef USE_ENUM_MODES
#define VNx3x8HFmode E_VNx3x8HFmode
#else
#define VNx3x8HFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx3x8HFmode))
#endif
  E_VNx3x8HImode,          /* config/riscv/riscv-seg-modes.def:35 */
#define HAVE_VNx3x8HImode
#ifdef USE_ENUM_MODES
#define VNx3x8HImode E_VNx3x8HImode
#else
#define VNx3x8HImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx3x8HImode))
#endif
  E_VNx3x2DImode,          /* config/riscv/riscv-seg-modes.def:52 */
#define HAVE_VNx3x2DImode
#ifdef USE_ENUM_MODES
#define VNx3x2DImode E_VNx3x2DImode
#else
#define VNx3x2DImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx3x2DImode))
#endif
  E_VNx3x4SImode,          /* config/riscv/riscv-seg-modes.def:63 */
#define HAVE_VNx3x4SImode
#ifdef USE_ENUM_MODES
#define VNx3x4SImode E_VNx3x4SImode
#else
#define VNx3x4SImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx3x4SImode))
#endif
  E_VNx3x16QImode,         /* config/riscv/riscv-seg-modes.def:68 */
#define HAVE_VNx3x16QImode
#ifdef USE_ENUM_MODES
#define VNx3x16QImode E_VNx3x16QImode
#else
#define VNx3x16QImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx3x16QImode))
#endif
  E_VNx3x4SFmode,          /* config/riscv/riscv-seg-modes.def:69 */
#define HAVE_VNx3x4SFmode
#ifdef USE_ENUM_MODES
#define VNx3x4SFmode E_VNx3x4SFmode
#else
#define VNx3x4SFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx3x4SFmode))
#endif
  E_VNx3x2DFmode,          /* config/riscv/riscv-seg-modes.def:77 */
#define HAVE_VNx3x2DFmode
#ifdef USE_ENUM_MODES
#define VNx3x2DFmode E_VNx3x2DFmode
#else
#define VNx3x2DFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx3x2DFmode))
#endif
  E_XImode,                /* config/riscv/riscv-modes.def:45 */
#define HAVE_XImode
#ifdef USE_ENUM_MODES
#define XImode E_XImode
#else
#define XImode (scalar_int_mode ((scalar_int_mode::from_int) E_XImode))
#endif
  E_VNx2x4DImode,          /* config/riscv/riscv-seg-modes.def:8 */
#define HAVE_VNx2x4DImode
#ifdef USE_ENUM_MODES
#define VNx2x4DImode E_VNx2x4DImode
#else
#define VNx2x4DImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x4DImode))
#endif
  E_VNx4x2DFmode,          /* config/riscv/riscv-seg-modes.def:13 */
#define HAVE_VNx4x2DFmode
#ifdef USE_ENUM_MODES
#define VNx4x2DFmode E_VNx4x2DFmode
#else
#define VNx4x2DFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx4x2DFmode))
#endif
  E_VNx4x8HImode,          /* config/riscv/riscv-seg-modes.def:14 */
#define HAVE_VNx4x8HImode
#ifdef USE_ENUM_MODES
#define VNx4x8HImode E_VNx4x8HImode
#else
#define VNx4x8HImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx4x8HImode))
#endif
  E_VNx2x16HFmode,         /* config/riscv/riscv-seg-modes.def:16 */
#define HAVE_VNx2x16HFmode
#ifdef USE_ENUM_MODES
#define VNx2x16HFmode E_VNx2x16HFmode
#else
#define VNx2x16HFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x16HFmode))
#endif
  E_VNx2x16HImode,         /* config/riscv/riscv-seg-modes.def:21 */
#define HAVE_VNx2x16HImode
#ifdef USE_ENUM_MODES
#define VNx2x16HImode E_VNx2x16HImode
#else
#define VNx2x16HImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x16HImode))
#endif
  E_VNx2x8SFmode,          /* config/riscv/riscv-seg-modes.def:23 */
#define HAVE_VNx2x8SFmode
#ifdef USE_ENUM_MODES
#define VNx2x8SFmode E_VNx2x8SFmode
#else
#define VNx2x8SFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x8SFmode))
#endif
  E_VNx4x4SImode,          /* config/riscv/riscv-seg-modes.def:28 */
#define HAVE_VNx4x4SImode
#ifdef USE_ENUM_MODES
#define VNx4x4SImode E_VNx4x4SImode
#else
#define VNx4x4SImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx4x4SImode))
#endif
  E_VNx4x4SFmode,          /* config/riscv/riscv-seg-modes.def:32 */
#define HAVE_VNx4x4SFmode
#ifdef USE_ENUM_MODES
#define VNx4x4SFmode E_VNx4x4SFmode
#else
#define VNx4x4SFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx4x4SFmode))
#endif
  E_VNx2x4DFmode,          /* config/riscv/riscv-seg-modes.def:37 */
#define HAVE_VNx2x4DFmode
#ifdef USE_ENUM_MODES
#define VNx2x4DFmode E_VNx2x4DFmode
#else
#define VNx2x4DFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x4DFmode))
#endif
  E_VNx4x8HFmode,          /* config/riscv/riscv-seg-modes.def:41 */
#define HAVE_VNx4x8HFmode
#ifdef USE_ENUM_MODES
#define VNx4x8HFmode E_VNx4x8HFmode
#else
#define VNx4x8HFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx4x8HFmode))
#endif
  E_VNx2x8SImode,          /* config/riscv/riscv-seg-modes.def:48 */
#define HAVE_VNx2x8SImode
#ifdef USE_ENUM_MODES
#define VNx2x8SImode E_VNx2x8SImode
#else
#define VNx2x8SImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x8SImode))
#endif
  E_VNx2x32QImode,         /* config/riscv/riscv-seg-modes.def:64 */
#define HAVE_VNx2x32QImode
#ifdef USE_ENUM_MODES
#define VNx2x32QImode E_VNx2x32QImode
#else
#define VNx2x32QImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x32QImode))
#endif
  E_VNx4x2DImode,          /* config/riscv/riscv-seg-modes.def:66 */
#define HAVE_VNx4x2DImode
#ifdef USE_ENUM_MODES
#define VNx4x2DImode E_VNx4x2DImode
#else
#define VNx4x2DImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx4x2DImode))
#endif
  E_VNx4x16QImode,         /* config/riscv/riscv-seg-modes.def:67 */
#define HAVE_VNx4x16QImode
#ifdef USE_ENUM_MODES
#define VNx4x16QImode E_VNx4x16QImode
#else
#define VNx4x16QImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx4x16QImode))
#endif
  E_VNx5x2DFmode,          /* config/riscv/riscv-seg-modes.def:30 */
#define HAVE_VNx5x2DFmode
#ifdef USE_ENUM_MODES
#define VNx5x2DFmode E_VNx5x2DFmode
#else
#define VNx5x2DFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx5x2DFmode))
#endif
  E_VNx5x4SFmode,          /* config/riscv/riscv-seg-modes.def:31 */
#define HAVE_VNx5x4SFmode
#ifdef USE_ENUM_MODES
#define VNx5x4SFmode E_VNx5x4SFmode
#else
#define VNx5x4SFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx5x4SFmode))
#endif
  E_VNx5x4SImode,          /* config/riscv/riscv-seg-modes.def:36 */
#define HAVE_VNx5x4SImode
#ifdef USE_ENUM_MODES
#define VNx5x4SImode E_VNx5x4SImode
#else
#define VNx5x4SImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx5x4SImode))
#endif
  E_VNx5x8HFmode,          /* config/riscv/riscv-seg-modes.def:53 */
#define HAVE_VNx5x8HFmode
#ifdef USE_ENUM_MODES
#define VNx5x8HFmode E_VNx5x8HFmode
#else
#define VNx5x8HFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx5x8HFmode))
#endif
  E_VNx5x8HImode,          /* config/riscv/riscv-seg-modes.def:54 */
#define HAVE_VNx5x8HImode
#ifdef USE_ENUM_MODES
#define VNx5x8HImode E_VNx5x8HImode
#else
#define VNx5x8HImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx5x8HImode))
#endif
  E_VNx5x2DImode,          /* config/riscv/riscv-seg-modes.def:59 */
#define HAVE_VNx5x2DImode
#ifdef USE_ENUM_MODES
#define VNx5x2DImode E_VNx5x2DImode
#else
#define VNx5x2DImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx5x2DImode))
#endif
  E_VNx5x16QImode,         /* config/riscv/riscv-seg-modes.def:60 */
#define HAVE_VNx5x16QImode
#ifdef USE_ENUM_MODES
#define VNx5x16QImode E_VNx5x16QImode
#else
#define VNx5x16QImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx5x16QImode))
#endif
  E_VNx6x4SFmode,          /* config/riscv/riscv-seg-modes.def:2 */
#define HAVE_VNx6x4SFmode
#ifdef USE_ENUM_MODES
#define VNx6x4SFmode E_VNx6x4SFmode
#else
#define VNx6x4SFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx6x4SFmode))
#endif
  E_VNx3x32QImode,         /* config/riscv/riscv-seg-modes.def:19 */
#define HAVE_VNx3x32QImode
#ifdef USE_ENUM_MODES
#define VNx3x32QImode E_VNx3x32QImode
#else
#define VNx3x32QImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx3x32QImode))
#endif
  E_VNx3x16HFmode,         /* config/riscv/riscv-seg-modes.def:27 */
#define HAVE_VNx3x16HFmode
#ifdef USE_ENUM_MODES
#define VNx3x16HFmode E_VNx3x16HFmode
#else
#define VNx3x16HFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx3x16HFmode))
#endif
  E_VNx3x16HImode,         /* config/riscv/riscv-seg-modes.def:33 */
#define HAVE_VNx3x16HImode
#ifdef USE_ENUM_MODES
#define VNx3x16HImode E_VNx3x16HImode
#else
#define VNx3x16HImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx3x16HImode))
#endif
  E_VNx6x4SImode,          /* config/riscv/riscv-seg-modes.def:34 */
#define HAVE_VNx6x4SImode
#ifdef USE_ENUM_MODES
#define VNx6x4SImode E_VNx6x4SImode
#else
#define VNx6x4SImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx6x4SImode))
#endif
  E_VNx6x8HFmode,          /* config/riscv/riscv-seg-modes.def:39 */
#define HAVE_VNx6x8HFmode
#ifdef USE_ENUM_MODES
#define VNx6x8HFmode E_VNx6x8HFmode
#else
#define VNx6x8HFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx6x8HFmode))
#endif
  E_VNx6x8HImode,          /* config/riscv/riscv-seg-modes.def:46 */
#define HAVE_VNx6x8HImode
#ifdef USE_ENUM_MODES
#define VNx6x8HImode E_VNx6x8HImode
#else
#define VNx6x8HImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx6x8HImode))
#endif
  E_VNx3x8SFmode,          /* config/riscv/riscv-seg-modes.def:61 */
#define HAVE_VNx3x8SFmode
#ifdef USE_ENUM_MODES
#define VNx3x8SFmode E_VNx3x8SFmode
#else
#define VNx3x8SFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx3x8SFmode))
#endif
  E_VNx6x16QImode,         /* config/riscv/riscv-seg-modes.def:62 */
#define HAVE_VNx6x16QImode
#ifdef USE_ENUM_MODES
#define VNx6x16QImode E_VNx6x16QImode
#else
#define VNx6x16QImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx6x16QImode))
#endif
  E_VNx3x8SImode,          /* config/riscv/riscv-seg-modes.def:65 */
#define HAVE_VNx3x8SImode
#ifdef USE_ENUM_MODES
#define VNx3x8SImode E_VNx3x8SImode
#else
#define VNx3x8SImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx3x8SImode))
#endif
  E_VNx6x2DImode,          /* config/riscv/riscv-seg-modes.def:70 */
#define HAVE_VNx6x2DImode
#ifdef USE_ENUM_MODES
#define VNx6x2DImode E_VNx6x2DImode
#else
#define VNx6x2DImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx6x2DImode))
#endif
  E_VNx3x4DImode,          /* config/riscv/riscv-seg-modes.def:71 */
#define HAVE_VNx3x4DImode
#ifdef USE_ENUM_MODES
#define VNx3x4DImode E_VNx3x4DImode
#else
#define VNx3x4DImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx3x4DImode))
#endif
  E_VNx3x4DFmode,          /* config/riscv/riscv-seg-modes.def:74 */
#define HAVE_VNx3x4DFmode
#ifdef USE_ENUM_MODES
#define VNx3x4DFmode E_VNx3x4DFmode
#else
#define VNx3x4DFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx3x4DFmode))
#endif
  E_VNx6x2DFmode,          /* config/riscv/riscv-seg-modes.def:75 */
#define HAVE_VNx6x2DFmode
#ifdef USE_ENUM_MODES
#define VNx6x2DFmode E_VNx6x2DFmode
#else
#define VNx6x2DFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx6x2DFmode))
#endif
  E_VNx7x16QImode,         /* config/riscv/riscv-seg-modes.def:1 */
#define HAVE_VNx7x16QImode
#ifdef USE_ENUM_MODES
#define VNx7x16QImode E_VNx7x16QImode
#else
#define VNx7x16QImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx7x16QImode))
#endif
  E_VNx7x4SImode,          /* config/riscv/riscv-seg-modes.def:6 */
#define HAVE_VNx7x4SImode
#ifdef USE_ENUM_MODES
#define VNx7x4SImode E_VNx7x4SImode
#else
#define VNx7x4SImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx7x4SImode))
#endif
  E_VNx7x4SFmode,          /* config/riscv/riscv-seg-modes.def:9 */
#define HAVE_VNx7x4SFmode
#ifdef USE_ENUM_MODES
#define VNx7x4SFmode E_VNx7x4SFmode
#else
#define VNx7x4SFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx7x4SFmode))
#endif
  E_VNx7x8HFmode,          /* config/riscv/riscv-seg-modes.def:29 */
#define HAVE_VNx7x8HFmode
#ifdef USE_ENUM_MODES
#define VNx7x8HFmode E_VNx7x8HFmode
#else
#define VNx7x8HFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx7x8HFmode))
#endif
  E_VNx7x2DImode,          /* config/riscv/riscv-seg-modes.def:51 */
#define HAVE_VNx7x2DImode
#ifdef USE_ENUM_MODES
#define VNx7x2DImode E_VNx7x2DImode
#else
#define VNx7x2DImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx7x2DImode))
#endif
  E_VNx7x2DFmode,          /* config/riscv/riscv-seg-modes.def:55 */
#define HAVE_VNx7x2DFmode
#ifdef USE_ENUM_MODES
#define VNx7x2DFmode E_VNx7x2DFmode
#else
#define VNx7x2DFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx7x2DFmode))
#endif
  E_VNx7x8HImode,          /* config/riscv/riscv-seg-modes.def:76 */
#define HAVE_VNx7x8HImode
#ifdef USE_ENUM_MODES
#define VNx7x8HImode E_VNx7x8HImode
#else
#define VNx7x8HImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx7x8HImode))
#endif
  E_ZImode,                /* config/riscv/riscv-modes.def:46 */
#define HAVE_ZImode
#ifdef USE_ENUM_MODES
#define ZImode E_ZImode
#else
#define ZImode (scalar_int_mode ((scalar_int_mode::from_int) E_ZImode))
#endif
  E_VNx8x8HFmode,          /* config/riscv/riscv-seg-modes.def:4 */
#define HAVE_VNx8x8HFmode
#ifdef USE_ENUM_MODES
#define VNx8x8HFmode E_VNx8x8HFmode
#else
#define VNx8x8HFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx8x8HFmode))
#endif
  E_VNx4x4DImode,          /* config/riscv/riscv-seg-modes.def:5 */
#define HAVE_VNx4x4DImode
#ifdef USE_ENUM_MODES
#define VNx4x4DImode E_VNx4x4DImode
#else
#define VNx4x4DImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx4x4DImode))
#endif
  E_VNx4x4DFmode,          /* config/riscv/riscv-seg-modes.def:10 */
#define HAVE_VNx4x4DFmode
#ifdef USE_ENUM_MODES
#define VNx4x4DFmode E_VNx4x4DFmode
#else
#define VNx4x4DFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx4x4DFmode))
#endif
  E_VNx8x8HImode,          /* config/riscv/riscv-seg-modes.def:11 */
#define HAVE_VNx8x8HImode
#ifdef USE_ENUM_MODES
#define VNx8x8HImode E_VNx8x8HImode
#else
#define VNx8x8HImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx8x8HImode))
#endif
  E_VNx2x8DImode,          /* config/riscv/riscv-seg-modes.def:15 */
#define HAVE_VNx2x8DImode
#ifdef USE_ENUM_MODES
#define VNx2x8DImode E_VNx2x8DImode
#else
#define VNx2x8DImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x8DImode))
#endif
  E_VNx2x16SImode,         /* config/riscv/riscv-seg-modes.def:18 */
#define HAVE_VNx2x16SImode
#ifdef USE_ENUM_MODES
#define VNx2x16SImode E_VNx2x16SImode
#else
#define VNx2x16SImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x16SImode))
#endif
  E_VNx2x16SFmode,         /* config/riscv/riscv-seg-modes.def:20 */
#define HAVE_VNx2x16SFmode
#ifdef USE_ENUM_MODES
#define VNx2x16SFmode E_VNx2x16SFmode
#else
#define VNx2x16SFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x16SFmode))
#endif
  E_VNx2x8DFmode,          /* config/riscv/riscv-seg-modes.def:22 */
#define HAVE_VNx2x8DFmode
#ifdef USE_ENUM_MODES
#define VNx2x8DFmode E_VNx2x8DFmode
#else
#define VNx2x8DFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x8DFmode))
#endif
  E_VNx8x16QImode,         /* config/riscv/riscv-seg-modes.def:24 */
#define HAVE_VNx8x16QImode
#ifdef USE_ENUM_MODES
#define VNx8x16QImode E_VNx8x16QImode
#else
#define VNx8x16QImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx8x16QImode))
#endif
  E_VNx8x2DImode,          /* config/riscv/riscv-seg-modes.def:38 */
#define HAVE_VNx8x2DImode
#ifdef USE_ENUM_MODES
#define VNx8x2DImode E_VNx8x2DImode
#else
#define VNx8x2DImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx8x2DImode))
#endif
  E_VNx2x32HImode,         /* config/riscv/riscv-seg-modes.def:40 */
#define HAVE_VNx2x32HImode
#ifdef USE_ENUM_MODES
#define VNx2x32HImode E_VNx2x32HImode
#else
#define VNx2x32HImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x32HImode))
#endif
  E_VNx4x8SFmode,          /* config/riscv/riscv-seg-modes.def:42 */
#define HAVE_VNx4x8SFmode
#ifdef USE_ENUM_MODES
#define VNx4x8SFmode E_VNx4x8SFmode
#else
#define VNx4x8SFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx4x8SFmode))
#endif
  E_VNx4x8SImode,          /* config/riscv/riscv-seg-modes.def:43 */
#define HAVE_VNx4x8SImode
#ifdef USE_ENUM_MODES
#define VNx4x8SImode E_VNx4x8SImode
#else
#define VNx4x8SImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx4x8SImode))
#endif
  E_VNx2x32HFmode,         /* config/riscv/riscv-seg-modes.def:45 */
#define HAVE_VNx2x32HFmode
#ifdef USE_ENUM_MODES
#define VNx2x32HFmode E_VNx2x32HFmode
#else
#define VNx2x32HFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x32HFmode))
#endif
  E_VNx8x2DFmode,          /* config/riscv/riscv-seg-modes.def:47 */
#define HAVE_VNx8x2DFmode
#ifdef USE_ENUM_MODES
#define VNx8x2DFmode E_VNx8x2DFmode
#else
#define VNx8x2DFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx8x2DFmode))
#endif
  E_VNx4x16HFmode,         /* config/riscv/riscv-seg-modes.def:50 */
#define HAVE_VNx4x16HFmode
#ifdef USE_ENUM_MODES
#define VNx4x16HFmode E_VNx4x16HFmode
#else
#define VNx4x16HFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx4x16HFmode))
#endif
  E_VNx4x16HImode,         /* config/riscv/riscv-seg-modes.def:56 */
#define HAVE_VNx4x16HImode
#ifdef USE_ENUM_MODES
#define VNx4x16HImode E_VNx4x16HImode
#else
#define VNx4x16HImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx4x16HImode))
#endif
  E_VNx4x32QImode,         /* config/riscv/riscv-seg-modes.def:57 */
#define HAVE_VNx4x32QImode
#ifdef USE_ENUM_MODES
#define VNx4x32QImode E_VNx4x32QImode
#else
#define VNx4x32QImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx4x32QImode))
#endif
  E_VNx2x64QImode,         /* config/riscv/riscv-seg-modes.def:58 */
#define HAVE_VNx2x64QImode
#ifdef USE_ENUM_MODES
#define VNx2x64QImode E_VNx2x64QImode
#else
#define VNx2x64QImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx2x64QImode))
#endif
  E_VNx8x4SFmode,          /* config/riscv/riscv-seg-modes.def:72 */
#define HAVE_VNx8x4SFmode
#ifdef USE_ENUM_MODES
#define VNx8x4SFmode E_VNx8x4SFmode
#else
#define VNx8x4SFmode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx8x4SFmode))
#endif
  E_VNx8x4SImode,          /* config/riscv/riscv-seg-modes.def:73 */
#define HAVE_VNx8x4SImode
#ifdef USE_ENUM_MODES
#define VNx8x4SImode E_VNx8x4SImode
#else
#define VNx8x4SImode (scalar_int_mode ((scalar_int_mode::from_int) E_VNx8x4SImode))
#endif
  E_QQmode,                /* machmode.def:234 */
#define HAVE_QQmode
#ifdef USE_ENUM_MODES
#define QQmode E_QQmode
#else
#define QQmode (scalar_mode ((scalar_mode::from_int) E_QQmode))
#endif
  E_HQmode,                /* machmode.def:235 */
#define HAVE_HQmode
#ifdef USE_ENUM_MODES
#define HQmode E_HQmode
#else
#define HQmode (scalar_mode ((scalar_mode::from_int) E_HQmode))
#endif
  E_SQmode,                /* machmode.def:236 */
#define HAVE_SQmode
#ifdef USE_ENUM_MODES
#define SQmode E_SQmode
#else
#define SQmode (scalar_mode ((scalar_mode::from_int) E_SQmode))
#endif
  E_DQmode,                /* machmode.def:237 */
#define HAVE_DQmode
#ifdef USE_ENUM_MODES
#define DQmode E_DQmode
#else
#define DQmode (scalar_mode ((scalar_mode::from_int) E_DQmode))
#endif
  E_TQmode,                /* machmode.def:238 */
#define HAVE_TQmode
#ifdef USE_ENUM_MODES
#define TQmode E_TQmode
#else
#define TQmode (scalar_mode ((scalar_mode::from_int) E_TQmode))
#endif
  E_UQQmode,               /* machmode.def:240 */
#define HAVE_UQQmode
#ifdef USE_ENUM_MODES
#define UQQmode E_UQQmode
#else
#define UQQmode (scalar_mode ((scalar_mode::from_int) E_UQQmode))
#endif
  E_UHQmode,               /* machmode.def:241 */
#define HAVE_UHQmode
#ifdef USE_ENUM_MODES
#define UHQmode E_UHQmode
#else
#define UHQmode (scalar_mode ((scalar_mode::from_int) E_UHQmode))
#endif
  E_USQmode,               /* machmode.def:242 */
#define HAVE_USQmode
#ifdef USE_ENUM_MODES
#define USQmode E_USQmode
#else
#define USQmode (scalar_mode ((scalar_mode::from_int) E_USQmode))
#endif
  E_UDQmode,               /* machmode.def:243 */
#define HAVE_UDQmode
#ifdef USE_ENUM_MODES
#define UDQmode E_UDQmode
#else
#define UDQmode (scalar_mode ((scalar_mode::from_int) E_UDQmode))
#endif
  E_UTQmode,               /* machmode.def:244 */
#define HAVE_UTQmode
#ifdef USE_ENUM_MODES
#define UTQmode E_UTQmode
#else
#define UTQmode (scalar_mode ((scalar_mode::from_int) E_UTQmode))
#endif
  E_HAmode,                /* machmode.def:246 */
#define HAVE_HAmode
#ifdef USE_ENUM_MODES
#define HAmode E_HAmode
#else
#define HAmode (scalar_mode ((scalar_mode::from_int) E_HAmode))
#endif
  E_SAmode,                /* machmode.def:247 */
#define HAVE_SAmode
#ifdef USE_ENUM_MODES
#define SAmode E_SAmode
#else
#define SAmode (scalar_mode ((scalar_mode::from_int) E_SAmode))
#endif
  E_DAmode,                /* machmode.def:248 */
#define HAVE_DAmode
#ifdef USE_ENUM_MODES
#define DAmode E_DAmode
#else
#define DAmode (scalar_mode ((scalar_mode::from_int) E_DAmode))
#endif
  E_TAmode,                /* machmode.def:249 */
#define HAVE_TAmode
#ifdef USE_ENUM_MODES
#define TAmode E_TAmode
#else
#define TAmode (scalar_mode ((scalar_mode::from_int) E_TAmode))
#endif
  E_UHAmode,               /* machmode.def:251 */
#define HAVE_UHAmode
#ifdef USE_ENUM_MODES
#define UHAmode E_UHAmode
#else
#define UHAmode (scalar_mode ((scalar_mode::from_int) E_UHAmode))
#endif
  E_USAmode,               /* machmode.def:252 */
#define HAVE_USAmode
#ifdef USE_ENUM_MODES
#define USAmode E_USAmode
#else
#define USAmode (scalar_mode ((scalar_mode::from_int) E_USAmode))
#endif
  E_UDAmode,               /* machmode.def:253 */
#define HAVE_UDAmode
#ifdef USE_ENUM_MODES
#define UDAmode E_UDAmode
#else
#define UDAmode (scalar_mode ((scalar_mode::from_int) E_UDAmode))
#endif
  E_UTAmode,               /* machmode.def:254 */
#define HAVE_UTAmode
#ifdef USE_ENUM_MODES
#define UTAmode E_UTAmode
#else
#define UTAmode (scalar_mode ((scalar_mode::from_int) E_UTAmode))
#endif
  E_HFmode,                /* config/riscv/riscv-modes.def:22 */
#define HAVE_HFmode
#ifdef USE_ENUM_MODES
#define HFmode E_HFmode
#else
#define HFmode (scalar_float_mode ((scalar_float_mode::from_int) E_HFmode))
#endif
  E_SFmode,                /* machmode.def:226 */
#define HAVE_SFmode
#ifdef USE_ENUM_MODES
#define SFmode E_SFmode
#else
#define SFmode (scalar_float_mode ((scalar_float_mode::from_int) E_SFmode))
#endif
  E_DFmode,                /* machmode.def:227 */
#define HAVE_DFmode
#ifdef USE_ENUM_MODES
#define DFmode E_DFmode
#else
#define DFmode (scalar_float_mode ((scalar_float_mode::from_int) E_DFmode))
#endif
  E_TFmode,                /* config/riscv/riscv-modes.def:23 */
#define HAVE_TFmode
#ifdef USE_ENUM_MODES
#define TFmode E_TFmode
#else
#define TFmode (scalar_float_mode ((scalar_float_mode::from_int) E_TFmode))
#endif
  E_SDmode,                /* machmode.def:267 */
#define HAVE_SDmode
#ifdef USE_ENUM_MODES
#define SDmode E_SDmode
#else
#define SDmode (scalar_float_mode ((scalar_float_mode::from_int) E_SDmode))
#endif
  E_DDmode,                /* machmode.def:268 */
#define HAVE_DDmode
#ifdef USE_ENUM_MODES
#define DDmode E_DDmode
#else
#define DDmode (scalar_float_mode ((scalar_float_mode::from_int) E_DDmode))
#endif
  E_TDmode,                /* machmode.def:269 */
#define HAVE_TDmode
#ifdef USE_ENUM_MODES
#define TDmode E_TDmode
#else
#define TDmode (scalar_float_mode ((scalar_float_mode::from_int) E_TDmode))
#endif
  E_CQImode,               /* machmode.def:262 */
#define HAVE_CQImode
#ifdef USE_ENUM_MODES
#define CQImode E_CQImode
#else
#define CQImode (complex_mode ((complex_mode::from_int) E_CQImode))
#endif
  E_CHImode,               /* machmode.def:262 */
#define HAVE_CHImode
#ifdef USE_ENUM_MODES
#define CHImode E_CHImode
#else
#define CHImode (complex_mode ((complex_mode::from_int) E_CHImode))
#endif
  E_CSImode,               /* machmode.def:262 */
#define HAVE_CSImode
#ifdef USE_ENUM_MODES
#define CSImode E_CSImode
#else
#define CSImode (complex_mode ((complex_mode::from_int) E_CSImode))
#endif
  E_CDImode,               /* machmode.def:262 */
#define HAVE_CDImode
#ifdef USE_ENUM_MODES
#define CDImode E_CDImode
#else
#define CDImode (complex_mode ((complex_mode::from_int) E_CDImode))
#endif
  E_CTImode,               /* machmode.def:262 */
#define HAVE_CTImode
#ifdef USE_ENUM_MODES
#define CTImode E_CTImode
#else
#define CTImode (complex_mode ((complex_mode::from_int) E_CTImode))
#endif
  E_CVNx2x2DFmode,         /* machmode.def:262 */
#define HAVE_CVNx2x2DFmode
#ifdef USE_ENUM_MODES
#define CVNx2x2DFmode E_CVNx2x2DFmode
#else
#define CVNx2x2DFmode (complex_mode ((complex_mode::from_int) E_CVNx2x2DFmode))
#endif
  E_CVNx2x4SFmode,         /* machmode.def:262 */
#define HAVE_CVNx2x4SFmode
#ifdef USE_ENUM_MODES
#define CVNx2x4SFmode E_CVNx2x4SFmode
#else
#define CVNx2x4SFmode (complex_mode ((complex_mode::from_int) E_CVNx2x4SFmode))
#endif
  E_CVNx2x8HImode,         /* machmode.def:262 */
#define HAVE_CVNx2x8HImode
#ifdef USE_ENUM_MODES
#define CVNx2x8HImode E_CVNx2x8HImode
#else
#define CVNx2x8HImode (complex_mode ((complex_mode::from_int) E_CVNx2x8HImode))
#endif
  E_CVNx2x4SImode,         /* machmode.def:262 */
#define HAVE_CVNx2x4SImode
#ifdef USE_ENUM_MODES
#define CVNx2x4SImode E_CVNx2x4SImode
#else
#define CVNx2x4SImode (complex_mode ((complex_mode::from_int) E_CVNx2x4SImode))
#endif
  E_CVNx2x8HFmode,         /* machmode.def:262 */
#define HAVE_CVNx2x8HFmode
#ifdef USE_ENUM_MODES
#define CVNx2x8HFmode E_CVNx2x8HFmode
#else
#define CVNx2x8HFmode (complex_mode ((complex_mode::from_int) E_CVNx2x8HFmode))
#endif
  E_CVNx2x16QImode,        /* machmode.def:262 */
#define HAVE_CVNx2x16QImode
#ifdef USE_ENUM_MODES
#define CVNx2x16QImode E_CVNx2x16QImode
#else
#define CVNx2x16QImode (complex_mode ((complex_mode::from_int) E_CVNx2x16QImode))
#endif
  E_CVNx2x2DImode,         /* machmode.def:262 */
#define HAVE_CVNx2x2DImode
#ifdef USE_ENUM_MODES
#define CVNx2x2DImode E_CVNx2x2DImode
#else
#define CVNx2x2DImode (complex_mode ((complex_mode::from_int) E_CVNx2x2DImode))
#endif
  E_COImode,               /* machmode.def:262 */
#define HAVE_COImode
#ifdef USE_ENUM_MODES
#define COImode E_COImode
#else
#define COImode (complex_mode ((complex_mode::from_int) E_COImode))
#endif
  E_CVNx3x2DFmode,         /* machmode.def:262 */
#define HAVE_CVNx3x2DFmode
#ifdef USE_ENUM_MODES
#define CVNx3x2DFmode E_CVNx3x2DFmode
#else
#define CVNx3x2DFmode (complex_mode ((complex_mode::from_int) E_CVNx3x2DFmode))
#endif
  E_CVNx3x4SFmode,         /* machmode.def:262 */
#define HAVE_CVNx3x4SFmode
#ifdef USE_ENUM_MODES
#define CVNx3x4SFmode E_CVNx3x4SFmode
#else
#define CVNx3x4SFmode (complex_mode ((complex_mode::from_int) E_CVNx3x4SFmode))
#endif
  E_CVNx3x16QImode,        /* machmode.def:262 */
#define HAVE_CVNx3x16QImode
#ifdef USE_ENUM_MODES
#define CVNx3x16QImode E_CVNx3x16QImode
#else
#define CVNx3x16QImode (complex_mode ((complex_mode::from_int) E_CVNx3x16QImode))
#endif
  E_CVNx3x4SImode,         /* machmode.def:262 */
#define HAVE_CVNx3x4SImode
#ifdef USE_ENUM_MODES
#define CVNx3x4SImode E_CVNx3x4SImode
#else
#define CVNx3x4SImode (complex_mode ((complex_mode::from_int) E_CVNx3x4SImode))
#endif
  E_CVNx3x2DImode,         /* machmode.def:262 */
#define HAVE_CVNx3x2DImode
#ifdef USE_ENUM_MODES
#define CVNx3x2DImode E_CVNx3x2DImode
#else
#define CVNx3x2DImode (complex_mode ((complex_mode::from_int) E_CVNx3x2DImode))
#endif
  E_CVNx3x8HImode,         /* machmode.def:262 */
#define HAVE_CVNx3x8HImode
#ifdef USE_ENUM_MODES
#define CVNx3x8HImode E_CVNx3x8HImode
#else
#define CVNx3x8HImode (complex_mode ((complex_mode::from_int) E_CVNx3x8HImode))
#endif
  E_CVNx3x8HFmode,         /* machmode.def:262 */
#define HAVE_CVNx3x8HFmode
#ifdef USE_ENUM_MODES
#define CVNx3x8HFmode E_CVNx3x8HFmode
#else
#define CVNx3x8HFmode (complex_mode ((complex_mode::from_int) E_CVNx3x8HFmode))
#endif
  E_CVNx4x16QImode,        /* machmode.def:262 */
#define HAVE_CVNx4x16QImode
#ifdef USE_ENUM_MODES
#define CVNx4x16QImode E_CVNx4x16QImode
#else
#define CVNx4x16QImode (complex_mode ((complex_mode::from_int) E_CVNx4x16QImode))
#endif
  E_CVNx4x2DImode,         /* machmode.def:262 */
#define HAVE_CVNx4x2DImode
#ifdef USE_ENUM_MODES
#define CVNx4x2DImode E_CVNx4x2DImode
#else
#define CVNx4x2DImode (complex_mode ((complex_mode::from_int) E_CVNx4x2DImode))
#endif
  E_CVNx2x32QImode,        /* machmode.def:262 */
#define HAVE_CVNx2x32QImode
#ifdef USE_ENUM_MODES
#define CVNx2x32QImode E_CVNx2x32QImode
#else
#define CVNx2x32QImode (complex_mode ((complex_mode::from_int) E_CVNx2x32QImode))
#endif
  E_CVNx2x8SImode,         /* machmode.def:262 */
#define HAVE_CVNx2x8SImode
#ifdef USE_ENUM_MODES
#define CVNx2x8SImode E_CVNx2x8SImode
#else
#define CVNx2x8SImode (complex_mode ((complex_mode::from_int) E_CVNx2x8SImode))
#endif
  E_CVNx4x8HFmode,         /* machmode.def:262 */
#define HAVE_CVNx4x8HFmode
#ifdef USE_ENUM_MODES
#define CVNx4x8HFmode E_CVNx4x8HFmode
#else
#define CVNx4x8HFmode (complex_mode ((complex_mode::from_int) E_CVNx4x8HFmode))
#endif
  E_CVNx2x4DFmode,         /* machmode.def:262 */
#define HAVE_CVNx2x4DFmode
#ifdef USE_ENUM_MODES
#define CVNx2x4DFmode E_CVNx2x4DFmode
#else
#define CVNx2x4DFmode (complex_mode ((complex_mode::from_int) E_CVNx2x4DFmode))
#endif
  E_CVNx4x4SFmode,         /* machmode.def:262 */
#define HAVE_CVNx4x4SFmode
#ifdef USE_ENUM_MODES
#define CVNx4x4SFmode E_CVNx4x4SFmode
#else
#define CVNx4x4SFmode (complex_mode ((complex_mode::from_int) E_CVNx4x4SFmode))
#endif
  E_CVNx4x4SImode,         /* machmode.def:262 */
#define HAVE_CVNx4x4SImode
#ifdef USE_ENUM_MODES
#define CVNx4x4SImode E_CVNx4x4SImode
#else
#define CVNx4x4SImode (complex_mode ((complex_mode::from_int) E_CVNx4x4SImode))
#endif
  E_CVNx2x8SFmode,         /* machmode.def:262 */
#define HAVE_CVNx2x8SFmode
#ifdef USE_ENUM_MODES
#define CVNx2x8SFmode E_CVNx2x8SFmode
#else
#define CVNx2x8SFmode (complex_mode ((complex_mode::from_int) E_CVNx2x8SFmode))
#endif
  E_CVNx2x16HImode,        /* machmode.def:262 */
#define HAVE_CVNx2x16HImode
#ifdef USE_ENUM_MODES
#define CVNx2x16HImode E_CVNx2x16HImode
#else
#define CVNx2x16HImode (complex_mode ((complex_mode::from_int) E_CVNx2x16HImode))
#endif
  E_CVNx2x16HFmode,        /* machmode.def:262 */
#define HAVE_CVNx2x16HFmode
#ifdef USE_ENUM_MODES
#define CVNx2x16HFmode E_CVNx2x16HFmode
#else
#define CVNx2x16HFmode (complex_mode ((complex_mode::from_int) E_CVNx2x16HFmode))
#endif
  E_CVNx4x8HImode,         /* machmode.def:262 */
#define HAVE_CVNx4x8HImode
#ifdef USE_ENUM_MODES
#define CVNx4x8HImode E_CVNx4x8HImode
#else
#define CVNx4x8HImode (complex_mode ((complex_mode::from_int) E_CVNx4x8HImode))
#endif
  E_CVNx4x2DFmode,         /* machmode.def:262 */
#define HAVE_CVNx4x2DFmode
#ifdef USE_ENUM_MODES
#define CVNx4x2DFmode E_CVNx4x2DFmode
#else
#define CVNx4x2DFmode (complex_mode ((complex_mode::from_int) E_CVNx4x2DFmode))
#endif
  E_CVNx2x4DImode,         /* machmode.def:262 */
#define HAVE_CVNx2x4DImode
#ifdef USE_ENUM_MODES
#define CVNx2x4DImode E_CVNx2x4DImode
#else
#define CVNx2x4DImode (complex_mode ((complex_mode::from_int) E_CVNx2x4DImode))
#endif
  E_CXImode,               /* machmode.def:262 */
#define HAVE_CXImode
#ifdef USE_ENUM_MODES
#define CXImode E_CXImode
#else
#define CXImode (complex_mode ((complex_mode::from_int) E_CXImode))
#endif
  E_CVNx5x16QImode,        /* machmode.def:262 */
#define HAVE_CVNx5x16QImode
#ifdef USE_ENUM_MODES
#define CVNx5x16QImode E_CVNx5x16QImode
#else
#define CVNx5x16QImode (complex_mode ((complex_mode::from_int) E_CVNx5x16QImode))
#endif
  E_CVNx5x2DImode,         /* machmode.def:262 */
#define HAVE_CVNx5x2DImode
#ifdef USE_ENUM_MODES
#define CVNx5x2DImode E_CVNx5x2DImode
#else
#define CVNx5x2DImode (complex_mode ((complex_mode::from_int) E_CVNx5x2DImode))
#endif
  E_CVNx5x8HImode,         /* machmode.def:262 */
#define HAVE_CVNx5x8HImode
#ifdef USE_ENUM_MODES
#define CVNx5x8HImode E_CVNx5x8HImode
#else
#define CVNx5x8HImode (complex_mode ((complex_mode::from_int) E_CVNx5x8HImode))
#endif
  E_CVNx5x8HFmode,         /* machmode.def:262 */
#define HAVE_CVNx5x8HFmode
#ifdef USE_ENUM_MODES
#define CVNx5x8HFmode E_CVNx5x8HFmode
#else
#define CVNx5x8HFmode (complex_mode ((complex_mode::from_int) E_CVNx5x8HFmode))
#endif
  E_CVNx5x4SImode,         /* machmode.def:262 */
#define HAVE_CVNx5x4SImode
#ifdef USE_ENUM_MODES
#define CVNx5x4SImode E_CVNx5x4SImode
#else
#define CVNx5x4SImode (complex_mode ((complex_mode::from_int) E_CVNx5x4SImode))
#endif
  E_CVNx5x4SFmode,         /* machmode.def:262 */
#define HAVE_CVNx5x4SFmode
#ifdef USE_ENUM_MODES
#define CVNx5x4SFmode E_CVNx5x4SFmode
#else
#define CVNx5x4SFmode (complex_mode ((complex_mode::from_int) E_CVNx5x4SFmode))
#endif
  E_CVNx5x2DFmode,         /* machmode.def:262 */
#define HAVE_CVNx5x2DFmode
#ifdef USE_ENUM_MODES
#define CVNx5x2DFmode E_CVNx5x2DFmode
#else
#define CVNx5x2DFmode (complex_mode ((complex_mode::from_int) E_CVNx5x2DFmode))
#endif
  E_CVNx6x2DFmode,         /* machmode.def:262 */
#define HAVE_CVNx6x2DFmode
#ifdef USE_ENUM_MODES
#define CVNx6x2DFmode E_CVNx6x2DFmode
#else
#define CVNx6x2DFmode (complex_mode ((complex_mode::from_int) E_CVNx6x2DFmode))
#endif
  E_CVNx3x4DFmode,         /* machmode.def:262 */
#define HAVE_CVNx3x4DFmode
#ifdef USE_ENUM_MODES
#define CVNx3x4DFmode E_CVNx3x4DFmode
#else
#define CVNx3x4DFmode (complex_mode ((complex_mode::from_int) E_CVNx3x4DFmode))
#endif
  E_CVNx3x4DImode,         /* machmode.def:262 */
#define HAVE_CVNx3x4DImode
#ifdef USE_ENUM_MODES
#define CVNx3x4DImode E_CVNx3x4DImode
#else
#define CVNx3x4DImode (complex_mode ((complex_mode::from_int) E_CVNx3x4DImode))
#endif
  E_CVNx6x2DImode,         /* machmode.def:262 */
#define HAVE_CVNx6x2DImode
#ifdef USE_ENUM_MODES
#define CVNx6x2DImode E_CVNx6x2DImode
#else
#define CVNx6x2DImode (complex_mode ((complex_mode::from_int) E_CVNx6x2DImode))
#endif
  E_CVNx3x8SImode,         /* machmode.def:262 */
#define HAVE_CVNx3x8SImode
#ifdef USE_ENUM_MODES
#define CVNx3x8SImode E_CVNx3x8SImode
#else
#define CVNx3x8SImode (complex_mode ((complex_mode::from_int) E_CVNx3x8SImode))
#endif
  E_CVNx6x16QImode,        /* machmode.def:262 */
#define HAVE_CVNx6x16QImode
#ifdef USE_ENUM_MODES
#define CVNx6x16QImode E_CVNx6x16QImode
#else
#define CVNx6x16QImode (complex_mode ((complex_mode::from_int) E_CVNx6x16QImode))
#endif
  E_CVNx3x8SFmode,         /* machmode.def:262 */
#define HAVE_CVNx3x8SFmode
#ifdef USE_ENUM_MODES
#define CVNx3x8SFmode E_CVNx3x8SFmode
#else
#define CVNx3x8SFmode (complex_mode ((complex_mode::from_int) E_CVNx3x8SFmode))
#endif
  E_CVNx6x8HImode,         /* machmode.def:262 */
#define HAVE_CVNx6x8HImode
#ifdef USE_ENUM_MODES
#define CVNx6x8HImode E_CVNx6x8HImode
#else
#define CVNx6x8HImode (complex_mode ((complex_mode::from_int) E_CVNx6x8HImode))
#endif
  E_CVNx6x8HFmode,         /* machmode.def:262 */
#define HAVE_CVNx6x8HFmode
#ifdef USE_ENUM_MODES
#define CVNx6x8HFmode E_CVNx6x8HFmode
#else
#define CVNx6x8HFmode (complex_mode ((complex_mode::from_int) E_CVNx6x8HFmode))
#endif
  E_CVNx6x4SImode,         /* machmode.def:262 */
#define HAVE_CVNx6x4SImode
#ifdef USE_ENUM_MODES
#define CVNx6x4SImode E_CVNx6x4SImode
#else
#define CVNx6x4SImode (complex_mode ((complex_mode::from_int) E_CVNx6x4SImode))
#endif
  E_CVNx3x16HImode,        /* machmode.def:262 */
#define HAVE_CVNx3x16HImode
#ifdef USE_ENUM_MODES
#define CVNx3x16HImode E_CVNx3x16HImode
#else
#define CVNx3x16HImode (complex_mode ((complex_mode::from_int) E_CVNx3x16HImode))
#endif
  E_CVNx3x16HFmode,        /* machmode.def:262 */
#define HAVE_CVNx3x16HFmode
#ifdef USE_ENUM_MODES
#define CVNx3x16HFmode E_CVNx3x16HFmode
#else
#define CVNx3x16HFmode (complex_mode ((complex_mode::from_int) E_CVNx3x16HFmode))
#endif
  E_CVNx3x32QImode,        /* machmode.def:262 */
#define HAVE_CVNx3x32QImode
#ifdef USE_ENUM_MODES
#define CVNx3x32QImode E_CVNx3x32QImode
#else
#define CVNx3x32QImode (complex_mode ((complex_mode::from_int) E_CVNx3x32QImode))
#endif
  E_CVNx6x4SFmode,         /* machmode.def:262 */
#define HAVE_CVNx6x4SFmode
#ifdef USE_ENUM_MODES
#define CVNx6x4SFmode E_CVNx6x4SFmode
#else
#define CVNx6x4SFmode (complex_mode ((complex_mode::from_int) E_CVNx6x4SFmode))
#endif
  E_CVNx7x8HImode,         /* machmode.def:262 */
#define HAVE_CVNx7x8HImode
#ifdef USE_ENUM_MODES
#define CVNx7x8HImode E_CVNx7x8HImode
#else
#define CVNx7x8HImode (complex_mode ((complex_mode::from_int) E_CVNx7x8HImode))
#endif
  E_CVNx7x2DFmode,         /* machmode.def:262 */
#define HAVE_CVNx7x2DFmode
#ifdef USE_ENUM_MODES
#define CVNx7x2DFmode E_CVNx7x2DFmode
#else
#define CVNx7x2DFmode (complex_mode ((complex_mode::from_int) E_CVNx7x2DFmode))
#endif
  E_CVNx7x2DImode,         /* machmode.def:262 */
#define HAVE_CVNx7x2DImode
#ifdef USE_ENUM_MODES
#define CVNx7x2DImode E_CVNx7x2DImode
#else
#define CVNx7x2DImode (complex_mode ((complex_mode::from_int) E_CVNx7x2DImode))
#endif
  E_CVNx7x8HFmode,         /* machmode.def:262 */
#define HAVE_CVNx7x8HFmode
#ifdef USE_ENUM_MODES
#define CVNx7x8HFmode E_CVNx7x8HFmode
#else
#define CVNx7x8HFmode (complex_mode ((complex_mode::from_int) E_CVNx7x8HFmode))
#endif
  E_CVNx7x4SFmode,         /* machmode.def:262 */
#define HAVE_CVNx7x4SFmode
#ifdef USE_ENUM_MODES
#define CVNx7x4SFmode E_CVNx7x4SFmode
#else
#define CVNx7x4SFmode (complex_mode ((complex_mode::from_int) E_CVNx7x4SFmode))
#endif
  E_CVNx7x4SImode,         /* machmode.def:262 */
#define HAVE_CVNx7x4SImode
#ifdef USE_ENUM_MODES
#define CVNx7x4SImode E_CVNx7x4SImode
#else
#define CVNx7x4SImode (complex_mode ((complex_mode::from_int) E_CVNx7x4SImode))
#endif
  E_CVNx7x16QImode,        /* machmode.def:262 */
#define HAVE_CVNx7x16QImode
#ifdef USE_ENUM_MODES
#define CVNx7x16QImode E_CVNx7x16QImode
#else
#define CVNx7x16QImode (complex_mode ((complex_mode::from_int) E_CVNx7x16QImode))
#endif
  E_CVNx8x4SImode,         /* machmode.def:262 */
#define HAVE_CVNx8x4SImode
#ifdef USE_ENUM_MODES
#define CVNx8x4SImode E_CVNx8x4SImode
#else
#define CVNx8x4SImode (complex_mode ((complex_mode::from_int) E_CVNx8x4SImode))
#endif
  E_CVNx8x4SFmode,         /* machmode.def:262 */
#define HAVE_CVNx8x4SFmode
#ifdef USE_ENUM_MODES
#define CVNx8x4SFmode E_CVNx8x4SFmode
#else
#define CVNx8x4SFmode (complex_mode ((complex_mode::from_int) E_CVNx8x4SFmode))
#endif
  E_CVNx2x64QImode,        /* machmode.def:262 */
#define HAVE_CVNx2x64QImode
#ifdef USE_ENUM_MODES
#define CVNx2x64QImode E_CVNx2x64QImode
#else
#define CVNx2x64QImode (complex_mode ((complex_mode::from_int) E_CVNx2x64QImode))
#endif
  E_CVNx4x32QImode,        /* machmode.def:262 */
#define HAVE_CVNx4x32QImode
#ifdef USE_ENUM_MODES
#define CVNx4x32QImode E_CVNx4x32QImode
#else
#define CVNx4x32QImode (complex_mode ((complex_mode::from_int) E_CVNx4x32QImode))
#endif
  E_CVNx4x16HImode,        /* machmode.def:262 */
#define HAVE_CVNx4x16HImode
#ifdef USE_ENUM_MODES
#define CVNx4x16HImode E_CVNx4x16HImode
#else
#define CVNx4x16HImode (complex_mode ((complex_mode::from_int) E_CVNx4x16HImode))
#endif
  E_CVNx4x16HFmode,        /* machmode.def:262 */
#define HAVE_CVNx4x16HFmode
#ifdef USE_ENUM_MODES
#define CVNx4x16HFmode E_CVNx4x16HFmode
#else
#define CVNx4x16HFmode (complex_mode ((complex_mode::from_int) E_CVNx4x16HFmode))
#endif
  E_CVNx8x2DFmode,         /* machmode.def:262 */
#define HAVE_CVNx8x2DFmode
#ifdef USE_ENUM_MODES
#define CVNx8x2DFmode E_CVNx8x2DFmode
#else
#define CVNx8x2DFmode (complex_mode ((complex_mode::from_int) E_CVNx8x2DFmode))
#endif
  E_CVNx2x32HFmode,        /* machmode.def:262 */
#define HAVE_CVNx2x32HFmode
#ifdef USE_ENUM_MODES
#define CVNx2x32HFmode E_CVNx2x32HFmode
#else
#define CVNx2x32HFmode (complex_mode ((complex_mode::from_int) E_CVNx2x32HFmode))
#endif
  E_CVNx4x8SImode,         /* machmode.def:262 */
#define HAVE_CVNx4x8SImode
#ifdef USE_ENUM_MODES
#define CVNx4x8SImode E_CVNx4x8SImode
#else
#define CVNx4x8SImode (complex_mode ((complex_mode::from_int) E_CVNx4x8SImode))
#endif
  E_CVNx4x8SFmode,         /* machmode.def:262 */
#define HAVE_CVNx4x8SFmode
#ifdef USE_ENUM_MODES
#define CVNx4x8SFmode E_CVNx4x8SFmode
#else
#define CVNx4x8SFmode (complex_mode ((complex_mode::from_int) E_CVNx4x8SFmode))
#endif
  E_CVNx2x32HImode,        /* machmode.def:262 */
#define HAVE_CVNx2x32HImode
#ifdef USE_ENUM_MODES
#define CVNx2x32HImode E_CVNx2x32HImode
#else
#define CVNx2x32HImode (complex_mode ((complex_mode::from_int) E_CVNx2x32HImode))
#endif
  E_CVNx8x2DImode,         /* machmode.def:262 */
#define HAVE_CVNx8x2DImode
#ifdef USE_ENUM_MODES
#define CVNx8x2DImode E_CVNx8x2DImode
#else
#define CVNx8x2DImode (complex_mode ((complex_mode::from_int) E_CVNx8x2DImode))
#endif
  E_CVNx8x16QImode,        /* machmode.def:262 */
#define HAVE_CVNx8x16QImode
#ifdef USE_ENUM_MODES
#define CVNx8x16QImode E_CVNx8x16QImode
#else
#define CVNx8x16QImode (complex_mode ((complex_mode::from_int) E_CVNx8x16QImode))
#endif
  E_CVNx2x8DFmode,         /* machmode.def:262 */
#define HAVE_CVNx2x8DFmode
#ifdef USE_ENUM_MODES
#define CVNx2x8DFmode E_CVNx2x8DFmode
#else
#define CVNx2x8DFmode (complex_mode ((complex_mode::from_int) E_CVNx2x8DFmode))
#endif
  E_CVNx2x16SFmode,        /* machmode.def:262 */
#define HAVE_CVNx2x16SFmode
#ifdef USE_ENUM_MODES
#define CVNx2x16SFmode E_CVNx2x16SFmode
#else
#define CVNx2x16SFmode (complex_mode ((complex_mode::from_int) E_CVNx2x16SFmode))
#endif
  E_CVNx2x16SImode,        /* machmode.def:262 */
#define HAVE_CVNx2x16SImode
#ifdef USE_ENUM_MODES
#define CVNx2x16SImode E_CVNx2x16SImode
#else
#define CVNx2x16SImode (complex_mode ((complex_mode::from_int) E_CVNx2x16SImode))
#endif
  E_CVNx2x8DImode,         /* machmode.def:262 */
#define HAVE_CVNx2x8DImode
#ifdef USE_ENUM_MODES
#define CVNx2x8DImode E_CVNx2x8DImode
#else
#define CVNx2x8DImode (complex_mode ((complex_mode::from_int) E_CVNx2x8DImode))
#endif
  E_CVNx8x8HImode,         /* machmode.def:262 */
#define HAVE_CVNx8x8HImode
#ifdef USE_ENUM_MODES
#define CVNx8x8HImode E_CVNx8x8HImode
#else
#define CVNx8x8HImode (complex_mode ((complex_mode::from_int) E_CVNx8x8HImode))
#endif
  E_CVNx4x4DFmode,         /* machmode.def:262 */
#define HAVE_CVNx4x4DFmode
#ifdef USE_ENUM_MODES
#define CVNx4x4DFmode E_CVNx4x4DFmode
#else
#define CVNx4x4DFmode (complex_mode ((complex_mode::from_int) E_CVNx4x4DFmode))
#endif
  E_CVNx4x4DImode,         /* machmode.def:262 */
#define HAVE_CVNx4x4DImode
#ifdef USE_ENUM_MODES
#define CVNx4x4DImode E_CVNx4x4DImode
#else
#define CVNx4x4DImode (complex_mode ((complex_mode::from_int) E_CVNx4x4DImode))
#endif
  E_CVNx8x8HFmode,         /* machmode.def:262 */
#define HAVE_CVNx8x8HFmode
#ifdef USE_ENUM_MODES
#define CVNx8x8HFmode E_CVNx8x8HFmode
#else
#define CVNx8x8HFmode (complex_mode ((complex_mode::from_int) E_CVNx8x8HFmode))
#endif
  E_CZImode,               /* machmode.def:262 */
#define HAVE_CZImode
#ifdef USE_ENUM_MODES
#define CZImode E_CZImode
#else
#define CZImode (complex_mode ((complex_mode::from_int) E_CZImode))
#endif
  E_HCmode,                /* machmode.def:264 */
#define HAVE_HCmode
#ifdef USE_ENUM_MODES
#define HCmode E_HCmode
#else
#define HCmode (complex_mode ((complex_mode::from_int) E_HCmode))
#endif
  E_SCmode,                /* machmode.def:264 */
#define HAVE_SCmode
#ifdef USE_ENUM_MODES
#define SCmode E_SCmode
#else
#define SCmode (complex_mode ((complex_mode::from_int) E_SCmode))
#endif
  E_DCmode,                /* machmode.def:264 */
#define HAVE_DCmode
#ifdef USE_ENUM_MODES
#define DCmode E_DCmode
#else
#define DCmode (complex_mode ((complex_mode::from_int) E_DCmode))
#endif
  E_TCmode,                /* machmode.def:264 */
#define HAVE_TCmode
#ifdef USE_ENUM_MODES
#define TCmode E_TCmode
#else
#define TCmode (complex_mode ((complex_mode::from_int) E_TCmode))
#endif
  E_V4QImode,              /* config/riscv/riscv-modes.def:26 */
#define HAVE_V4QImode
#ifdef USE_ENUM_MODES
#define V4QImode E_V4QImode
#else
#define V4QImode ((void) 0, E_V4QImode)
#endif
  E_V2HImode,              /* config/riscv/riscv-modes.def:26 */
#define HAVE_V2HImode
#ifdef USE_ENUM_MODES
#define V2HImode E_V2HImode
#else
#define V2HImode ((void) 0, E_V2HImode)
#endif
  E_V8QImode,              /* config/riscv/riscv-modes.def:27 */
#define HAVE_V8QImode
#ifdef USE_ENUM_MODES
#define V8QImode E_V8QImode
#else
#define V8QImode ((void) 0, E_V8QImode)
#endif
  E_V4HImode,              /* config/riscv/riscv-modes.def:27 */
#define HAVE_V4HImode
#ifdef USE_ENUM_MODES
#define V4HImode E_V4HImode
#else
#define V4HImode ((void) 0, E_V4HImode)
#endif
  E_V2SImode,              /* config/riscv/riscv-modes.def:27 */
#define HAVE_V2SImode
#ifdef USE_ENUM_MODES
#define V2SImode E_V2SImode
#else
#define V2SImode ((void) 0, E_V2SImode)
#endif
  E_V1DImode,              /* config/riscv/riscv-modes.def:41 */
#define HAVE_V1DImode
#ifdef USE_ENUM_MODES
#define V1DImode E_V1DImode
#else
#define V1DImode ((void) 0, E_V1DImode)
#endif
  E_V16QImode,             /* config/riscv/riscv-modes.def:28 */
#define HAVE_V16QImode
#ifdef USE_ENUM_MODES
#define V16QImode E_V16QImode
#else
#define V16QImode ((void) 0, E_V16QImode)
#endif
  E_V8HImode,              /* config/riscv/riscv-modes.def:28 */
#define HAVE_V8HImode
#ifdef USE_ENUM_MODES
#define V8HImode E_V8HImode
#else
#define V8HImode ((void) 0, E_V8HImode)
#endif
  E_V4SImode,              /* config/riscv/riscv-modes.def:28 */
#define HAVE_V4SImode
#ifdef USE_ENUM_MODES
#define V4SImode E_V4SImode
#else
#define V4SImode ((void) 0, E_V4SImode)
#endif
  E_V2DImode,              /* config/riscv/riscv-modes.def:28 */
#define HAVE_V2DImode
#ifdef USE_ENUM_MODES
#define V2DImode E_V2DImode
#else
#define V2DImode ((void) 0, E_V2DImode)
#endif
  E_V1TImode,              /* config/riscv/riscv-modes.def:42 */
#define HAVE_V1TImode
#ifdef USE_ENUM_MODES
#define V1TImode E_V1TImode
#else
#define V1TImode ((void) 0, E_V1TImode)
#endif
  E_V32QImode,             /* config/riscv/riscv-modes.def:29 */
#define HAVE_V32QImode
#ifdef USE_ENUM_MODES
#define V32QImode E_V32QImode
#else
#define V32QImode ((void) 0, E_V32QImode)
#endif
  E_V16HImode,             /* config/riscv/riscv-modes.def:29 */
#define HAVE_V16HImode
#ifdef USE_ENUM_MODES
#define V16HImode E_V16HImode
#else
#define V16HImode ((void) 0, E_V16HImode)
#endif
  E_V8SImode,              /* config/riscv/riscv-modes.def:29 */
#define HAVE_V8SImode
#ifdef USE_ENUM_MODES
#define V8SImode E_V8SImode
#else
#define V8SImode ((void) 0, E_V8SImode)
#endif
  E_V4DImode,              /* config/riscv/riscv-modes.def:29 */
#define HAVE_V4DImode
#ifdef USE_ENUM_MODES
#define V4DImode E_V4DImode
#else
#define V4DImode ((void) 0, E_V4DImode)
#endif
  E_V2TImode,              /* config/riscv/riscv-modes.def:29 */
#define HAVE_V2TImode
#ifdef USE_ENUM_MODES
#define V2TImode E_V2TImode
#else
#define V2TImode ((void) 0, E_V2TImode)
#endif
  E_V64QImode,             /* config/riscv/riscv-modes.def:30 */
#define HAVE_V64QImode
#ifdef USE_ENUM_MODES
#define V64QImode E_V64QImode
#else
#define V64QImode ((void) 0, E_V64QImode)
#endif
  E_V32HImode,             /* config/riscv/riscv-modes.def:30 */
#define HAVE_V32HImode
#ifdef USE_ENUM_MODES
#define V32HImode E_V32HImode
#else
#define V32HImode ((void) 0, E_V32HImode)
#endif
  E_V16SImode,             /* config/riscv/riscv-modes.def:30 */
#define HAVE_V16SImode
#ifdef USE_ENUM_MODES
#define V16SImode E_V16SImode
#else
#define V16SImode ((void) 0, E_V16SImode)
#endif
  E_V8DImode,              /* config/riscv/riscv-modes.def:30 */
#define HAVE_V8DImode
#ifdef USE_ENUM_MODES
#define V8DImode E_V8DImode
#else
#define V8DImode ((void) 0, E_V8DImode)
#endif
  E_V4TImode,              /* config/riscv/riscv-modes.def:30 */
#define HAVE_V4TImode
#ifdef USE_ENUM_MODES
#define V4TImode E_V4TImode
#else
#define V4TImode ((void) 0, E_V4TImode)
#endif
  E_V128QImode,            /* config/riscv/riscv-modes.def:31 */
#define HAVE_V128QImode
#ifdef USE_ENUM_MODES
#define V128QImode E_V128QImode
#else
#define V128QImode ((void) 0, E_V128QImode)
#endif
  E_V64HImode,             /* config/riscv/riscv-modes.def:31 */
#define HAVE_V64HImode
#ifdef USE_ENUM_MODES
#define V64HImode E_V64HImode
#else
#define V64HImode ((void) 0, E_V64HImode)
#endif
  E_V32SImode,             /* config/riscv/riscv-modes.def:31 */
#define HAVE_V32SImode
#ifdef USE_ENUM_MODES
#define V32SImode E_V32SImode
#else
#define V32SImode ((void) 0, E_V32SImode)
#endif
  E_V16DImode,             /* config/riscv/riscv-modes.def:31 */
#define HAVE_V16DImode
#ifdef USE_ENUM_MODES
#define V16DImode E_V16DImode
#else
#define V16DImode ((void) 0, E_V16DImode)
#endif
  E_V8TImode,              /* config/riscv/riscv-modes.def:31 */
#define HAVE_V8TImode
#ifdef USE_ENUM_MODES
#define V8TImode E_V8TImode
#else
#define V8TImode ((void) 0, E_V8TImode)
#endif
  E_V4HFmode,              /* config/riscv/riscv-modes.def:33 */
#define HAVE_V4HFmode
#ifdef USE_ENUM_MODES
#define V4HFmode E_V4HFmode
#else
#define V4HFmode ((void) 0, E_V4HFmode)
#endif
  E_V2SFmode,              /* config/riscv/riscv-modes.def:33 */
#define HAVE_V2SFmode
#ifdef USE_ENUM_MODES
#define V2SFmode E_V2SFmode
#else
#define V2SFmode ((void) 0, E_V2SFmode)
#endif
  E_V1DFmode,              /* config/riscv/riscv-modes.def:39 */
#define HAVE_V1DFmode
#ifdef USE_ENUM_MODES
#define V1DFmode E_V1DFmode
#else
#define V1DFmode ((void) 0, E_V1DFmode)
#endif
  E_V8HFmode,              /* config/riscv/riscv-modes.def:34 */
#define HAVE_V8HFmode
#ifdef USE_ENUM_MODES
#define V8HFmode E_V8HFmode
#else
#define V8HFmode ((void) 0, E_V8HFmode)
#endif
  E_V4SFmode,              /* config/riscv/riscv-modes.def:34 */
#define HAVE_V4SFmode
#ifdef USE_ENUM_MODES
#define V4SFmode E_V4SFmode
#else
#define V4SFmode ((void) 0, E_V4SFmode)
#endif
  E_V2DFmode,              /* config/riscv/riscv-modes.def:34 */
#define HAVE_V2DFmode
#ifdef USE_ENUM_MODES
#define V2DFmode E_V2DFmode
#else
#define V2DFmode ((void) 0, E_V2DFmode)
#endif
  E_V1TFmode,              /* config/riscv/riscv-modes.def:40 */
#define HAVE_V1TFmode
#ifdef USE_ENUM_MODES
#define V1TFmode E_V1TFmode
#else
#define V1TFmode ((void) 0, E_V1TFmode)
#endif
  E_V16HFmode,             /* config/riscv/riscv-modes.def:35 */
#define HAVE_V16HFmode
#ifdef USE_ENUM_MODES
#define V16HFmode E_V16HFmode
#else
#define V16HFmode ((void) 0, E_V16HFmode)
#endif
  E_V8SFmode,              /* config/riscv/riscv-modes.def:35 */
#define HAVE_V8SFmode
#ifdef USE_ENUM_MODES
#define V8SFmode E_V8SFmode
#else
#define V8SFmode ((void) 0, E_V8SFmode)
#endif
  E_V4DFmode,              /* config/riscv/riscv-modes.def:35 */
#define HAVE_V4DFmode
#ifdef USE_ENUM_MODES
#define V4DFmode E_V4DFmode
#else
#define V4DFmode ((void) 0, E_V4DFmode)
#endif
  E_V2TFmode,              /* config/riscv/riscv-modes.def:35 */
#define HAVE_V2TFmode
#ifdef USE_ENUM_MODES
#define V2TFmode E_V2TFmode
#else
#define V2TFmode ((void) 0, E_V2TFmode)
#endif
  E_V32HFmode,             /* config/riscv/riscv-modes.def:36 */
#define HAVE_V32HFmode
#ifdef USE_ENUM_MODES
#define V32HFmode E_V32HFmode
#else
#define V32HFmode ((void) 0, E_V32HFmode)
#endif
  E_V16SFmode,             /* config/riscv/riscv-modes.def:36 */
#define HAVE_V16SFmode
#ifdef USE_ENUM_MODES
#define V16SFmode E_V16SFmode
#else
#define V16SFmode ((void) 0, E_V16SFmode)
#endif
  E_V8DFmode,              /* config/riscv/riscv-modes.def:36 */
#define HAVE_V8DFmode
#ifdef USE_ENUM_MODES
#define V8DFmode E_V8DFmode
#else
#define V8DFmode ((void) 0, E_V8DFmode)
#endif
  E_V4TFmode,              /* config/riscv/riscv-modes.def:36 */
#define HAVE_V4TFmode
#ifdef USE_ENUM_MODES
#define V4TFmode E_V4TFmode
#else
#define V4TFmode ((void) 0, E_V4TFmode)
#endif
  E_V64HFmode,             /* config/riscv/riscv-modes.def:37 */
#define HAVE_V64HFmode
#ifdef USE_ENUM_MODES
#define V64HFmode E_V64HFmode
#else
#define V64HFmode ((void) 0, E_V64HFmode)
#endif
  E_V32SFmode,             /* config/riscv/riscv-modes.def:37 */
#define HAVE_V32SFmode
#ifdef USE_ENUM_MODES
#define V32SFmode E_V32SFmode
#else
#define V32SFmode ((void) 0, E_V32SFmode)
#endif
  E_V16DFmode,             /* config/riscv/riscv-modes.def:37 */
#define HAVE_V16DFmode
#ifdef USE_ENUM_MODES
#define V16DFmode E_V16DFmode
#else
#define V16DFmode ((void) 0, E_V16DFmode)
#endif
  E_V8TFmode,              /* config/riscv/riscv-modes.def:37 */
#define HAVE_V8TFmode
#ifdef USE_ENUM_MODES
#define V8TFmode E_V8TFmode
#else
#define V8TFmode ((void) 0, E_V8TFmode)
#endif
  MAX_MACHINE_MODE,

  MIN_MODE_RANDOM = E_VOIDmode,
  MAX_MODE_RANDOM = E_BLKmode,

  MIN_MODE_CC = E_CCmode,
  MAX_MODE_CC = E_CCmode,

  MIN_MODE_INT = E_QImode,
  MAX_MODE_INT = E_VNx8x4SImode,

  MIN_MODE_PARTIAL_INT = E_VOIDmode,
  MAX_MODE_PARTIAL_INT = E_VOIDmode,

  MIN_MODE_FRACT = E_QQmode,
  MAX_MODE_FRACT = E_TQmode,

  MIN_MODE_UFRACT = E_UQQmode,
  MAX_MODE_UFRACT = E_UTQmode,

  MIN_MODE_ACCUM = E_HAmode,
  MAX_MODE_ACCUM = E_TAmode,

  MIN_MODE_UACCUM = E_UHAmode,
  MAX_MODE_UACCUM = E_UTAmode,

  MIN_MODE_FLOAT = E_HFmode,
  MAX_MODE_FLOAT = E_TFmode,

  MIN_MODE_DECIMAL_FLOAT = E_SDmode,
  MAX_MODE_DECIMAL_FLOAT = E_TDmode,

  MIN_MODE_COMPLEX_INT = E_CQImode,
  MAX_MODE_COMPLEX_INT = E_CZImode,

  MIN_MODE_COMPLEX_FLOAT = E_HCmode,
  MAX_MODE_COMPLEX_FLOAT = E_TCmode,

  MIN_MODE_VECTOR_BOOL = E_VOIDmode,
  MAX_MODE_VECTOR_BOOL = E_VOIDmode,

  MIN_MODE_VECTOR_INT = E_V4QImode,
  MAX_MODE_VECTOR_INT = E_V8TImode,

  MIN_MODE_VECTOR_FRACT = E_VOIDmode,
  MAX_MODE_VECTOR_FRACT = E_VOIDmode,

  MIN_MODE_VECTOR_UFRACT = E_VOIDmode,
  MAX_MODE_VECTOR_UFRACT = E_VOIDmode,

  MIN_MODE_VECTOR_ACCUM = E_VOIDmode,
  MAX_MODE_VECTOR_ACCUM = E_VOIDmode,

  MIN_MODE_VECTOR_UACCUM = E_VOIDmode,
  MAX_MODE_VECTOR_UACCUM = E_VOIDmode,

  MIN_MODE_VECTOR_FLOAT = E_V4HFmode,
  MAX_MODE_VECTOR_FLOAT = E_V8TFmode,

  NUM_MACHINE_MODES = MAX_MACHINE_MODE
};

#define CONST_MODE_NUNITS const
#define CONST_MODE_PRECISION const
#define CONST_MODE_SIZE const
#define CONST_MODE_UNIT_SIZE const
#define CONST_MODE_BASE_ALIGN const
#define CONST_MODE_IBIT const
#define CONST_MODE_FBIT const

#define BITS_PER_UNIT (8)
#define MAX_BITSIZE_MODE_ANY_INT (128*BITS_PER_UNIT)
#define MAX_BITSIZE_MODE_ANY_MODE (256*BITS_PER_UNIT)
#define NUM_INT_N_ENTS 1
#define NUM_POLY_INT_COEFFS 1

#endif /* insn-modes.h */
