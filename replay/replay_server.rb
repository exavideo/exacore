require 'rubygems'
require 'patchbay'
require 'irb'
require 'json'

require '/home/armena/object_ids'

require_relative 'replay_app'
require_relative '../input/shuttlepro'

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

class ReplayLocalControl < ShuttleProInput
    def initialize(app)
        @app = app
        @current_event = @app.each_source.map { |src| nil }
        @current_preview_source = 0
        super()
    end

    attr_accessor :shot_target
    attr_accessor :current_event

    def on_shuttle(value)
        case value
        when 0
            @app.program.stop
        when 1
            @app.program.set_speed(1,4)
        when 2
            @app.program.set_speed(1,3)
        when 3
            @app.program.set_speed(1,2)
        when 4
            @app.program.set_speed(1,1)
        when 5
            @app.program.set_speed(3,2)
        when 6
            @app.program.set_speed(2,1)
        end
    end

    def on_jog(value)
        @app.preview.seek(value * 5)
    end

    def on_button_down(button)
        case button
        when 257
            @app.program.stop
        when 259
            @app.program.shot = @app.preview.shot
        when 260
            preview_source 0
        when 261
            preview_source 1
        when 262
            preview_source 2
        when 263
            preview_source 3
        when 264
            preview_source 4
        when 269
            capture_event
        when 270
            send_event_to_web_interface
        end
    end

    def preview_source(source)
        @current_preview_source = source
        @app.preview.shot = current_event[source] if current_event[source]
    end

    def capture_event
        @current_event = @app.each_source.map { |src| src.make_shot_now }
        @app.preview.shot = current_event[@current_preview_source]
    end

    def send_event_to_web_interface
        current_event.each do |shot|
            shot_target.send_shot(shot)
        end
    end

    def start_irb
        #IRB.start_session(binding())
        @app.start_irb
    end
end

class ReplayServer < Patchbay
    # Get all shots currently saved.
    get '/shots.json' do
        render :json => shots.each_with_index.map { |x, i| 
            json_obj = x.make_json;  
            json_obj[:id] = i;
            json_obj
        } .to_json
    end

    # Add a new shot.
    post '/shots' do
        shots << inbound_shot
        json = inbound_shot.make_json
        json[:id] = shots.length - 1
        render :json => json
    end

    # Throw this shot up on the local operator's preview.
    put '/preview_shot.json' do
        replay_app.preview.shot = inbound_shot
        render :json => ''
    end

    # Roll this shot right now
    put '/roll_shot.json' do
        replay_app.program.shot = inbound_shot
        render :json => ''
    end

    # Preview the start of a shot.
    get '/shots/:id/preview.jpg' do
        render :jpg => shots[params[:id].to_i].preview
    end

    # Get source information.
    get '/sources.json' do
        x = 0
        render :json => replay_app.each_source.map do |source|
            { :id => source.persist_id, :name => source.name }
            x += 1
        end
    end

    # Preview what's on the source right now.
    get '/sources/:id/preview.jpg' do
        source = Object.from_persist_id(params[:id].to_i)
        shot = source.make_shot_now
        render :jpg => shot.preview
    end

    # Preview a given timecode for a given source.
    get '/sources/:id/:timecode/preview.jpg' do
        shot = ReplayShot.new
        shot.source = Object.from_persist_id(params[:id].to_i)
        shot.start = params[:timecode].to_i
        render :jpg => shot.preview
    end

    self.files_dir = "public_html/"

    # List of shots currently saved.
    def shots
        @shots ||= []
        @shots
    end

    # Save a new shot.
    def send_shot(shot)
        shots << shot
    end

    attr_accessor :replay_app

private
    def inbound_shot
        unless params[:inbound_shot]
            inp = environment['rack.input']
            inp.rewind
            params[:inbound_shot] = Replay::ReplayShot.from_json(inp.read)
        end

        params[:inbound_shot]
    end
end

server = ReplayServer.new
control = ReplayLocalControl.new(app)
control.shot_target = server
server.replay_app = app
Thread.new { server.run }
control.start_irb

