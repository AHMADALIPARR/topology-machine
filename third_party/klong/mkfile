# mkfile for Plan 9
# Nils M Holm, 2017

</$objtype/mkfile

TARG=		kg
OFILES=		kg.$O s9core.$O
CFLAGS=		$CFLAGS -Dplan9
CLEANFILES=	klong.image lib/klong.image

# Modules to load into image file
OPTIONS=	-l nstat -l nplot

all:V:	kg klong.image

kg:	$O.out
	cp $prereq $target

klong.image:	kg
	./kg -n $(OPTIONS) -o klong.image

test:V:
	./kg -n test.kg

</sys/src/cmd/mkone
