
-- A solution contains projects, and defines the available configurations
solution "graphicsByExample"
   targetdir ( "bin" )
   configurations { "Debug", "Release"}

   flags { "Unicode" , "NoPCH"}

   srcDirs = os.matchdirs("src/*")

   for i, projectName in ipairs(srcDirs) do

       -- A project defines one build target
       project (path.getname(projectName))
          kind "ConsoleApp"
          location (projectName)
          language "C++"
          configuration { "windows" }
             buildoptions ""
             linkoptions { "/NODEFAULTLIB:msvcrt" } -- https://github.com/yuriks/robotic/blob/master/premake5.lua
          configuration { "linux" }
             buildoptions "-std=c++11" --http://industriousone.com/topic/xcode4-c11-build-option
             toolset "gcc"
          configuration {}

          files { path.join(projectName, "**.h"), path.join(projectName, "**.cpp") } -- build all .h and .cpp files recursively
          excludes { "./graphics_dependencies/**" }  -- don't build files in graphics_dependencies/


          -- where are header files?
          configuration "windows"
          includedirs {
                        "./graphics_dependencies/SDL2/include",
                        "./graphics_dependencies/glew/include"
                      }
          configuration { "linux" }
          includedirs {
                        "/usr/include/SDL2",
                      }
          configuration {}


          -- what libraries need linking to
          configuration "windows"
             links { "SDL2", "SDL2main", "opengl32", "glew32" }
          configuration "linux"
             links { "SDL2", "SDL2main", "GL", "GLEW" }
          configuration {}


          -- where are libraries?
          configuration "windows"
          libdirs {
                    "./graphics_dependencies/glew/lib/Release/Win32",
                    "./graphics_dependencies/SDL2/lib/win32"
                  }
          configuration "linux"
                   -- should be installed as in ./graphics_dependencies/README.asciidoc
          configuration {}


          configuration "*Debug"
             defines { "DEBUG" }
             flags { "Symbols" }
             optimize "Off"
             targetsuffix "-debug"


          configuration "*Release"
             defines { "NDEBUG" }
             optimize "On"

          configuration "windows"
             postbuildcommands { ".\\post-build-event.bat" }
   end
