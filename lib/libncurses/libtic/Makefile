# Build special version of ncurses for tic(1)
LIB=	tic

# keep at low warns, hostlib
WARNS=	1

ZTIC=	yes
CFLAGS+=	-DDRAGONFLY_NATIVE

ENABLE_WIDEC=	yes

afterdepend: libtic.na

# this lib is used in place, it is not installed anywhere
install:

.include "${.CURDIR}/../Makefile.sources"
.include <bsd.hostlib.mk>
