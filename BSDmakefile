# Copyright (c) 2025 Mamadou Babaei
#
# Author: Mamadou Babaei <info@babaei.net>
#


# pmake might add -J (private)
FLAGS=${.MAKEFLAGS:C/\-J ([0-9]+,?)+//W}

all: .DEFAULT
.DEFAULT:
	@which gmake > /dev/null 2>&1 || \
		(echo "BSD Make is not supported; The build requires GNU Make." && exit 1)
	@gmake ${.FLAGS} ${.TARGETS}
