#ifndef CARBIO_DESCRIPTOR_TRAITS_H
#define CARBIO_DESCRIPTOR_TRAITS_H

#if defined(__unix) || defined(__unix__) || defined(__unix)
#include "descriptor_traits_unix.h"
#else
#error Unsupported platform!
#endif

#endif // CARBIO_DESCRIPTOR_TRAITS_H
