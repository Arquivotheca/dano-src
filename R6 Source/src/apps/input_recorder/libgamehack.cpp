#include <support/SupportDefs.h>
#include <app/Message.h>
#include <interface/Point.h>
#include <input_server_p/input_server_private.h>

void
set_mouse_position(int32 x, int32 y) {
        BMessage reply;
        BMessage command(IS_SET_MOUSE_POSITION);
        command.AddPoint(IS_WHERE, BPoint(x, y));

        _control_input_server_(&command, &reply);
}

