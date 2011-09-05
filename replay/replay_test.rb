require 'rubygems'
require 'backports'
require_relative 'replay_app'
require_relative '../input/shuttlepro'
require "irb"

iadp = Replay::create_decklink_input_adapter(1, 0, 0, Replay::RawFrame::CbYCrY8422)

app = Replay::ReplayApp.new

app.add_source(:input => iadp, :file => 'd1', :name => 'CAM 1')
app.start

module IRB # :nodoc:
    def self.start_session(binding)
        unless @__initialized
            args = ARGV
            ARGV.replace(ARGV.dup)
            IRB.setup(nil)
            ARGV.replace(args)
            @__initialized = true
        end

        workspace = WorkSpace.new(binding)

        irb = Irb.new(workspace)

        @CONF[:IRB_RC].call(irb.context) if @CONF[:IRB_RC]
        @CONF[:MAIN_CONTEXT] = irb.context

        catch(:IRB_EXIT) do
            irb.eval_input
        end
    end
end

class ReplayControl < ShuttleProInput
    def initialize(app)
        @app = app
        super()
    end

    def on_shuttle(value)
        case value
        when 0
            @app.roll_stop
        when 1
            @app.roll_speed(1,4)
        when 2
            @app.roll_speed(1,3)
        when 3
            @app.roll_speed(1,2)
        when 4
            @app.roll_speed(1,1)
        when 5
            @app.roll_speed(3,2)
        when 6
            @app.roll_speed(2,1)
        end

    end

    def on_jog(value)
        @app.seek_preview(value * 5)
    end

    def on_button_down(button)
        case button
        when 257
            @app.roll_stop
        when 259
            @app.roll_start_from_preview
        when 260
            @app.preview_camera(0)
        when 261
            @app.preview_camera(1)
        when 262
            @app.preview_camera(2)
        when 263
            @app.preview_camera(3)
        when 264
            @app.preview_camera(4)
        when 269
            @app.capture_event
        end
    end

    def start_irb
        #IRB.start_session(binding())
        @app.start_irb
    end
end

control = ReplayControl.new(app)
control.start_irb

