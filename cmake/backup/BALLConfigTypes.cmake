INCLUDE(CheckTypeSize)

IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
	SET(BALL_64BIT_ARCHITECTURE TRUE CACHE INTERNAL "Architecture-bits")
ELSE()
	SET(BALL_64BIT_ARCHITECTURE FALSE CACHE INTERNAL "Architecture-bits")
ENDIF()

## Some useful type sizes
CHECK_TYPE_SIZE("char"               BALL_CHAR_SIZE)
CHECK_TYPE_SIZE("short"              BALL_SHORT_SIZE)
CHECK_TYPE_SIZE("int"                BALL_INT_SIZE)
CHECK_TYPE_SIZE("long"               BALL_LONG_SIZE)
CHECK_TYPE_SIZE("size_t"             BALL_SIZE_T_SIZE)
CHECK_TYPE_SIZE("void*"              BALL_POINTER_SIZE)
CHECK_TYPE_SIZE("unsigned short"     BALL_USHORT_SIZE)
CHECK_TYPE_SIZE("unsigned int"       BALL_UINT_SIZE)
CHECK_TYPE_SIZE("unsigned long"      BALL_ULONG_SIZE)
CHECK_TYPE_SIZE("unsigned long long" BALL_ULONGLONG_SIZE)
CHECK_TYPE_SIZE("float"              BALL_FLOAT_SIZE)
CHECK_TYPE_SIZE("double"             BALL_DOUBLE_SIZE)

## Find a suitable integer types of 64 bit length
CHECK_TYPE_SIZE("uint64_t" BALL_UINT64_T_SIZE)
IF (HAVE_BALL_UINT64_T_SIZE)
	SET(BALL_INT64 int64_t)
	SET(BALL_UINT64 uint64_t)
	SET(BALL_ULONG64_TYPE uint64_t)
	SET(BALL_LONG64_TYPE int64_t)
ELSE()
	## Ok, we have to do this the hard way
	IF (BALL_UINT_SIZE MATCHES "8")
		SET(BALL_INT64        "int")
		SET(BALL_UINT64       "unsigned int")
		SET(BALL_ULONG64_TYPE "unsigned int")
		SET(BALL_LONG64_TYPE "int")
	ELSEIF(BALL_ULONG_SIZE MATCHES "8")
		SET(BALL_INT64        "long")
		SET(BALL_UINT64       "unsigned long")
		SET(BALL_ULONG64_TYPE "unsigned long")
		SET(BALL_LONG64_TYPE "long")
	ELSEIF(BALL_ULONGLONG_SIZE MATCHES "8")
		SET(BALL_INT64        "long long")
		SET(BALL_UINT64       "unsigned long long")
		SET(BALL_ULONG64_TYPE "unsigned long long")
		SET(BALL_LONG64_TYPE "long long")
	ELSE()
		MESSAGE(SEND_ERROR "Could not find appropriate numeric type for 64bit integers!")
	ENDIF()
ENDIF()

## Define a suitable pointer type
IF (BALL_POINTER_SIZE MATCHES ${BALL_UINT_SIZE})
	SET(BALL_POINTERSIZEUINT_TYPE "unsigned int")
ELSEIF (BALL_POINTER_SIZE MATCHES ${BALL_ULONG_SIZE})
	SET(BALL_POINTERSIZEUINT_TYPE "unsigned long")
ELSEIF (BALL_POINTER_SIZE MATCHES ${BALL_SIZE_T_SIZE})
	SET(BALL_POINTERSIZEUINT_TYPE "size_t")
ELSE()
	MESSAGE(SEND_ERROR "Could not find appropriate integer type of same size as void*")
ENDIF()

## Define a 32-bit type for Index and Size
## NOTE: this seems plain *wrong*! SIZE_TYPE and INDEX_TYPE need to be adapted to the
##       pointer size, and should not just be 32 bit types!
CHECK_TYPE_SIZE("uint32_t" BALL_UINT32_T_SIZE)
IF (HAVE_BALL_UINT32_T_SIZE)
	SET(BALL_INT32      "int32_t")
	SET(BALL_UINT32     "uint32_t")
	SET(BALL_SIZE_TYPE  "uint32_t")
	SET(BALL_INDEX_TYPE "int32_t")
ELSEIF (BALL_INT_SIZE MATCHES "4")
	SET(BALL_INT32      "int")
	SET(BALL_UINT32     "unsigned int")
	SET(BALL_SIZE_TYPE  "unsigned int")
	SET(BALL_INDEX_TYPE "int")
ELSEIF (BALL_LONG_SIZE MATCHES "4")
	SET(BALL_INT32      "long")
	SET(BALL_UINT32     "unsigned long")
	SET(BALL_SIZE_TYPE  "unsigned long")
	SET(BALL_INDEX_TYPE "long")
ELSE()
	MESSAGE(SEND_ERROR "Could not find appropriate numeric type for 32bit int")
ENDIF()

CHECK_TYPE_SIZE("uint16_t" BALL_UINT16_T_SIZE)
IF(HAVE_BALL_UINT16_T_SIZE)
	SET(BALL_INT16  "int16_t")
	SET(BALL_UINT16 "uint16_t")
ELSEIF (BALL_SHORT_SIZE MATCHES "2")
	SET(BALL_INT16  "short")
	SET(BALL_UINT16 "unsigned short")
ELSEIF (BALL_INT_SIZE MATCHES "2")
	SET(BALL_INT16  "int")
	SET(BALL_UINT16 "unsigned int")
ELSEIF (BALL_CHAR_SIZE MATCHES "2")
	SET(BALL_INT16  "char")
	SET(BALL_UINT16 "unsigned char")
ELSE()
	MESSAGE(SEND_ERROR "Could not find appropriate numeric type for 16bit int")
ENDIF()