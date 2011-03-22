/*
 * Copyright 2011 Andrew H. Armenia.
 * 
 * This file is part of openreplay.
 * 
 * openreplay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * openreplay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with openreplay.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "adapter.h"
#include "types.h"
#include "thread.h"
#include <string.h>

class PipeOutputAdapter : public OutputAdapter, public Thread {
    public:
        PipeOutputAdapter(const char *cmd) : _input_pipe(4) {
            _cmd = strdup(cmd);
            start_thread( );
        }

        virtual Pipe<RawFrame *> &input_pipe( ) { return _input_pipe; }
        
        virtual ~PipeOutputAdapter( ) {
            
        }

    protected:
        Pipe<RawFrame *> _input_pipe;
        char *_cmd;
        RawFrame *frame;

        void run_thread(void) {
            pid_t child;
            int pipefds[2];

            if (pipe(pipefds) < 0) {
                perror("pipe");
                throw std::runtime_error("PipeOutputAdapter: pipe failed");
            }

            child = fork( );
            if (child < 0) {
                perror("fork");
                throw std::runtime_error("PipeOutputAdapter: fork failed");
            } else if (child == 0) {
                close(pipefds[1]);
                dup2(pipefds[0], STDIN_FILENO);
                execlp("/bin/sh", "/bin/sh", "-c", _cmd, NULL);
                perror("exec");
                throw std::runtime_error("PipeOutputAdapter: exec failed");
            } else {
                close(pipefds[0]);
                for (;;) {
                    if (_input_pipe.get(frame) == 0) { 
                        break;  
                    }
                    if (frame->write_to_fd(pipefds[1]) != 1) {
                        throw std::runtime_error(
                                "PipeOutputAdapter: broken pipe?"
                        );
                    }
                    delete frame;
                }
            }
        }
};

OutputAdapter *create_pipe_output_adapter(const char *cmd) {
    return new PipeOutputAdapter(cmd);
}
