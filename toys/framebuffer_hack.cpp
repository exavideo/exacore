#include "framebuffer_display_surface.h"
#include "freetype_font.h"

int main( ) {
    FramebufferDisplaySurface dpy;
    RawFrame *text;

    text = new RawFrame(255, 255, RawFrame::BGRAn8);

    for (;;) {
        dpy.draw->alpha_key(0, 0, text, 255);
    }

    delete text;
    return 0;
}
