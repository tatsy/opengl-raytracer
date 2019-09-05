#pragma once

// -----------------------------------------------------------------------------
// Forward declaration
// -----------------------------------------------------------------------------

class VertexArrayObject;
class FramebufferObject;
class ShaderProgram;
class Texture;
class TextureBuffer;

// -----------------------------------------------------------------------------
// API export macro
// -----------------------------------------------------------------------------

#if (defined(WIN32) || defined(_WIN32) || defined(WINCE) || defined(__CYGWIN__))
#   if defined(GLRT_API_EXPORT)
#       define GLRT_API __declspec(dllexport)
#       define GLRT_IMPORTS
#   else
#       define GLRT_API
#       define GLRT_IMPORTS __declspec(dllimport)
#   endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#   define GLRT_API __attribute__((visibility ("default")))
#   define GLRT_IMPORTS
#else
#   define GLRT_API
#   define GLRT_IMPORTS
#endif
