/* catch out-of-bounds replay buffer accesses, repackage as Ruby exception */
%exception {
    try {
        $action
    } catch (const ReplayFrameNotFoundException &) {
        static VALUE myerror = rb_define_class("ReplayFrameNotFoundError", rb_eStandardError);
        rb_raise(myerror, "Frame not found.");
    }
}

%include "stdint.i"
%include "replay_types.i"
%include "replay_shot.i"
%include "replay_frame_extractor.i"
%include "replay_buffer.i"
%include "replay_multiviewer.i"
%include "replay_preview.i"
%include "replay_playout.i"
%include "replay_ingest.i"
%include "replay_mjpeg_ingest.i"
%include "replay_gamedata.i"
%include "replay_playout_filter.i"
%include "replay_playout_image_filter.i"

