
all:
	@echo "#################################################"
	@echo "#                                               #"
	@echo "#   Compiling '3rdparty libs' Configs as below  #"
	@echo "#                                               #"
	@echo "#################################################"
	@for x in `find ./ -maxdepth 2 -mindepth 2 -name "Makefile" `; \
	    do cd `dirname $$x`; if [ $$? ]; then make || exit 1; cd -; fi; done

clean:
	@for x in `find ./ -maxdepth 2 -mindepth 2 -name "Makefile" `; \
	    do cd `dirname $$x`; if [ $$? ]; then make clean; cd -; fi; done
