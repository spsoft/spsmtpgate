
#--------------------------------------------------------------------

ifeq ($(origin version), undefined)
	version = 0.1
endif

#--------------------------------------------------------------------

all: 3rd
	@( cd spsmtpgate; make )

3rd:
	@( cd spmime; make )
	@( cd spnetkit; make )
	@( cd spserver; make )

dist: clean spsmtpgate-$(version).src.tar.gz

spsmtpgate-$(version).src.tar.gz:
	@find . -type f | grep -v CVS | grep -v .svn | sed s:^./:spsmtpgate-$(version)/: > MANIFEST
	@(cd ..; ln -s spsmtpgate spsmtpgate-$(version))
	(cd ..; tar cvf - `cat spsmtpgate/MANIFEST` | gzip > spsmtpgate/spsmtpgate-$(version).src.tar.gz)
	@(cd ..; rm spsmtpgate-$(version))

clean:
	@( cd spmime; make clean )
	@( cd spnetkit; make clean )
	@( cd spserver; make clean )
	@( cd spsmtpgate; make clean )

