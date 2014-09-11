AC_INIT(9pmod,1.0)
if test -r $OFED_HEADERS/include/linux/compat-2.6.h; then
	AC_MSG_NOTICE([compat-2.6.h is present.])
	AC_DEFINE(HAVE_COMPAT_RDMA, 1)
else
	AC_MSG_NOTICE([compat-2.6.h is not present.])
fi
if grep -qs ib_qp_type $OFED_HEADERS/include/rdma/rdma_cm.h; then
	AC_MSG_NOTICE([RDMA CM uses ib_qp_type.])
	AC_DEFINE(HAVE_RDMA_CM_QP_TYPE,1)
else
	AC_MSG_NOTICE([RDMA CM does not use ib_qp_type.])
fi


if diff confdefs.h config.h >/dev/null 2>&1; then
	echo config.h is unchanged.
else
	echo writing to config.h
	cp confdefs.h config.h
fi
