#pragma once

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

static const double Pi = 4.0 * std::atan(1.0);

// -----------------------------------------------------------------------------
// OpenGL libraries
// -----------------------------------------------------------------------------

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <glm/gtx/transform.hpp>
#include <GLFW/glfw3.h>

// -----------------------------------------------------------------------------
// OpenMP settings
// -----------------------------------------------------------------------------

#if defined(_OPENMP)
#include <omp.h>
#if defined(_MSC_VER)
#define omp_parallel_for __pragma(omp parallel for) for
#else
#define omp_parallel_for _Pragma("omp parallel for") for
#endif
#else
#define omp_parallel_for for
#define omp_get_num_threads() 1
#define omp_get_max_threads() 1
#define omp_get_thread_num() 0
#define omp_set_num_threads()
#endif

// -----------------------------------------------------------------------------
// Message handlers
// -----------------------------------------------------------------------------

#define Info(...)                     \
    do {                              \
        std::cout << "[INFO] ";       \
        fprintf(stdout, __VA_ARGS__); \
        std::cout << std::endl;       \
    } while (false);

#define Warning(...)                  \
    do {                              \
        std::cerr << "[WARNING] ";    \
        fprintf(stderr, __VA_ARGS__); \
        std::cerr << std::endl;       \
    } while (false);

#define FatalError(...)               \
    do {                              \
        std::cerr << "[ERROR] ";      \
        fprintf(stderr, __VA_ARGS__); \
        std::cerr << std::endl;       \
        std::abort();                 \
    } while (false);

#ifndef NDEBUG
#define Debug(...)                    \
    do {                              \
        std::cerr << "[DEBUG] ";      \
        fprintf(stdout, __VA_ARGS__); \
        std::cerr << std::endl;       \
    } while (false);
#else
#define Debug(...)
#endif
