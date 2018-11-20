#
# RH_USES_DPKG
#
# Determine if the target is a dpkg system or rpm
#
AC_DEFUN([RH_USES_DPKG], [
AC_CACHE_CHECK([if this distro uses dpkg], rh_cv_uses_dpkg, [
rh_cv_uses_dpkg="no"
AS_CASE([$(lsb_release -i -s 2>/dev/null)],
        [Ubuntu | Debian], [rh_cv_uses_dpkg="yes"])
])
uses_dpkg=$rh_cv_uses_dpkg
])
