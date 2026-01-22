#pragma once
#include <stdint.h>

#ifndef _DEBUG_CONTAINER_LEVEL

#ifdef _DEBUG
#define _DEBUG_CONTAINER_LEVEL 1
#else
#define _DEBUG_CONTAINER_LEVEL 0
#endif

#endif

#ifndef _DEBUG_ITERATOR_LEVEL

#ifdef _DEBUG
#define _DEBUG_ITERATOR_LEVEL 1
#else
#define _DEBUG_ITERATOR_LEVEL 0
#endif

#endif