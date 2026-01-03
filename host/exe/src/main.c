#include <stdio.h>
#include <windows.h>

#include "GLFW/glfw3.h"
#include "error/fatal.h"
#include "interface.h"
#include "reloading.h"
#include "window/window_input.h"
#include "window/window.h"

int main()
{
    if(!initGl())
    {
        printf("Error: Failed to init OpenGL constext!\n");
        return 1;
    }
    const char *lib_path = "./build/bin/libhot.dll";
    const char *cpy_path = "./build/bin/_libhot.dll";

    struct LoadedModule hctx = hr_initialize(lib_path, cpy_path);
    hr_set_ctx(&hctx);
    if(!hr_load()) return 1;

    struct HostContext host;
    host.window_ctx = w_get_window_ctx();
    host.delta_time = 0;
    host.root = NULL;

    host.window = (struct WindowContext)
    { .width= 0, .height= 0, .ratio= 1, .is_framebuffer_resized= 0 };

    if(
            !arena_init(&host.arenas.main_arena, MB(16)) ||
            !arena_init(&host.arenas.frame_arena, MB(1)) ||
            !arena_init(&host.arenas.level_arena, MB(1))
      )
    {
        fatal("failed to init arenas!");
    }

    double last_frame_time = glfwGetTime();

    i_initialize();
    if(!hctx.fuctions.init(&host))
    {
        fatal("init failed!");
        exit(0);
    }

    while(!window_should_close())
    {
        glfwPollEvents();
        double frame_time = glfwGetTime();
        host.delta_time =   (float)(frame_time - last_frame_time);
        host.time =         (float)frame_time;
        last_frame_time =   frame_time;

        host.inputinfo = i_get_info();
        

        if(hr_newer_available()) 
        {
            hctx.fuctions.deinit(&host);
            while(!hr_load()){}
            hctx.fuctions.init(&host);
            printf("Game reloaded !\n");
        }

        host.window.is_framebuffer_resized = 0;
        if(is_framebuffer_resized())
        {
            int w, h;
            window_get_size(&w, &h);
            host.window = (struct WindowContext)
            {
                .width= w, .height= h,
                .ratio = (float)w/(float)h,
                .is_framebuffer_resized = 1,
            };
            clear_framebuffer_resized();
        }

        hctx.fuctions.update(&host);

        i_clear();
    }

    arena_deinit(&host.arenas.main_arena);
    arena_deinit(&host.arenas.frame_arena);
    arena_deinit(&host.arenas.level_arena);

    return 0;
}
