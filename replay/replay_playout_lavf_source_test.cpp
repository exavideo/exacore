#include "replay_playout_lavf_source.h"
#include <unistd.h>

int main( ) {
    ReplayPlayoutLavfSource src("/home/rpitv/rollout/clarkson_60s_mjpeg.mov");
    Rational speed;
    ReplayPlayoutFrame data;

    for (;;) {
        src.read_frame(data, speed);
        //data.video_data->write_to_fd(STDOUT_FILENO);
        delete data.video_data;
        //data.audio_data->write_to_fd(STDOUT_FILENO);
        delete data.audio_data;
    }
}
