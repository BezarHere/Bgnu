#pragma once
#include <string>
#include <errno.h>

enum class Error
{
	Ok,
	Failure,

	NoData,
	InvalidType,

	NotImplemented,
};

struct ErrorReport
{
	Error code = Error::Ok;
	std::string message;
};

inline constexpr const char *errno_str(const errno_t error) {
	switch (error)
	{
	case EPERM:
		return "EPERM";
	case ENOENT:
		return "ENOENT";
	case ESRCH:
		return "ESRCH";
	case EINTR:
		return "EINTR";
	case EIO:
		return "EIO";
	case ENXIO:
		return "ENXIO";
	case E2BIG:
		return "E2BIG";
	case ENOEXEC:
		return "ENOEXEC";
	case EBADF:
		return "EBADF";
	case ECHILD:
		return "ECHILD";
	case EAGAIN:
		return "EAGAIN";
	case ENOMEM:
		return "ENOMEM";
	case EACCES:
		return "EACCES";
	case EFAULT:
		return "EFAULT";
	case EBUSY:
		return "EBUSY";
	case EEXIST:
		return "EEXIST";
	case EXDEV:
		return "EXDEV";
	case ENODEV:
		return "ENODEV";
	case ENOTDIR:
		return "ENOTDIR";
	case EISDIR:
		return "EISDIR";
	case ENFILE:
		return "ENFILE";
	case EMFILE:
		return "EMFILE";
	case ENOTTY:
		return "ENOTTY";
	case EFBIG:
		return "EFBIG";
	case ENOSPC:
		return "ENOSPC";
	case ESPIPE:
		return "ESPIPE";
	case EROFS:
		return "EROFS";
	case EMLINK:
		return "EMLINK";
	case EPIPE:
		return "EPIPE";
	case EDOM:
		return "EDOM";
	case EDEADLK:
		return "EDEADLK";
	case ENAMETOOLONG:
		return "ENAMETOOLONG";
	case ENOLCK:
		return "ENOLCK";
	case ENOSYS:
		return "ENOSYS";
	case ENOTEMPTY:
		return "ENOTEMPTY";
	case EINVAL:
		return "EINVAL";
	case ERANGE:
		return "ERANGE";
	case EILSEQ:
		return "EILSEQ";
	case STRUNCATE:
		return "STRUNCATE";
	case ENOTSUP:
		return "ENOTSUP";
	case EAFNOSUPPORT:
		return "EAFNOSUPPORT";
	case EADDRINUSE:
		return "EADDRINUSE";
	case EADDRNOTAVAIL:
		return "EADDRNOTAVAIL";
	case EISCONN:
		return "EISCONN";
	case ENOBUFS:
		return "ENOBUFS";
	case ECONNABORTED:
		return "ECONNABORTED";
	case EALREADY:
		return "EALREADY";
	case ECONNREFUSED:
		return "ECONNREFUSED";
	case ECONNRESET:
		return "ECONNRESET";
	case EDESTADDRREQ:
		return "EDESTADDRREQ";
	case EHOSTUNREACH:
		return "EHOSTUNREACH";
	case EMSGSIZE:
		return "EMSGSIZE";
	case ENETDOWN:
		return "ENETDOWN";
	case ENETRESET:
		return "ENETRESET";
	case ENETUNREACH:
		return "ENETUNREACH";
	case ENOPROTOOPT:
		return "ENOPROTOOPT";
	case ENOTSOCK:
		return "ENOTSOCK";
	case ENOTCONN:
		return "ENOTCONN";
	case ECANCELED:
		return "ECANCELED";
	case EINPROGRESS:
		return "EINPROGRESS";
	case EOPNOTSUPP:
		return "EOPNOTSUPP";
	case EWOULDBLOCK:
		return "EWOULDBLOCK";
	case EOWNERDEAD:
		return "EOWNERDEAD";
	case EPROTO:
		return "EPROTO";
	case EPROTONOSUPPORT:
		return "EPROTONOSUPPORT";
	case EBADMSG:
		return "EBADMSG";
	case EIDRM:
		return "EIDRM";
	case ENODATA:
		return "ENODATA";
	case ENOLINK:
		return "ENOLINK";
	case ENOMSG:
		return "ENOMSG";
	case ENOSR:
		return "ENOSR";
	case ENOSTR:
		return "ENOSTR";
	case ENOTRECOVERABLE:
		return "ENOTRECOVERABLE";
	case ETIME:
		return "ETIME";
	case ETXTBSY:
		return "ETXTBSY";
	case ETIMEDOUT:
		return "ETIMEDOUT";
	case ELOOP:
		return "ELOOP";
	case EPROTOTYPE:
		return "EPROTOTYPE";
	case EOVERFLOW:
		return "EOVERFLOW";
	default:
		return "UNKNOWN_ERROR";
	}
}
