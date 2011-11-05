require 'rubygems'
require 'patchbay'
require 'irb'
require 'json'
require 'thin'

require_relative 'object_ids'

require_relative 'replay_app'
require_relative '../input/shuttlepro'

iadp1 = Replay::create_decklink_input_adapter(1, 0, 0, Replay::RawFrame::CbYCrY8422)
#iadp2 = Replay::create_decklink_input_adapter(2, 0, 0, Replay::RawFrame::CbYCrY8422)
#iadp3 = Replay::create_decklink_input_adapter(3, 0, 0, Replay::RawFrame::CbYCrY8422)
#iadp4 = Replay::create_decklink_input_adapter(4, 0, 0, Replay::RawFrame::CbYCrY8422)
#iadp5 = Replay::create_decklink_input_adapter(5, 0, 0, Replay::RawFrame::CbYCrY8422)
#iadp6 = Replay::create_decklink_input_adapter(6, 0, 0, Replay::RawFrame::CbYCrY8422)

app = Replay::ReplayApp.new
app.game_data.clock = "13:37"

#app.add_source(:mjpeg_cmd => 'nc 192.168.216.1 12345', :file => '/root/d5', :name => 'WIRELESS 1')
app.add_source(:input => iadp1, :file => '/mnt/cam1/d1', :name => 'CAM 1')
#app.add_source(:input => iadp2, :file => '/mnt/cam2/d2', :name => 'CAM 2')
#app.add_source(:input => iadp3, :file => '/mnt/cam3/d3', :name => 'CAM 3')
#app.add_source(:input => iadp4, :file => '/mnt/cam4/d4', :name => 'CAM 4')
#app.add_source(:input => iadp5, :file => '/root/d5', :name => 'HYPERMOTION')
#app.add_source(:input => iadp6, :file => '/root/d6', :name => 'CAM 6')
app.program.add_svg_dsk(IO.read('/root/instantreplay.svg'), 0, 0)
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
        @shifted = 0
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
        when 256
            @app.preview.mark_in
        when 257
            @app.program.stop
        when 258
            @app.preview.mark_in
            @app.program.shot = @app.preview.shot
            send_preview_to_web_interface
        when 259
            @app.preview.mark_out
        when 260
            if @shifted == 1
                preview_source 4
            else
                preview_source 0
            end
        when 261
            if @shifted == 1
                preview_source 5
            else
                preview_source 1
            end
        when 262
            preview_source 2
        when 263
            preview_source 3
        when 264
            @shifted = 2
        when 269
            capture_event
        when 270
            send_preview_to_web_interface
            #send_event_to_web_interface
        end

        if @shifted == 2
            @shifted = 1
        elsif @shifted == 1
            @shifted = 0
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

    def send_preview_to_web_interface
        shot_target.send_shot(@app.preview.shot)
    end

    def start_irb
        #IRB.start_session(binding())
        @app.start_irb
    end
end

class MjpegIterator
    def initialize(source, start, length)
        @source = source
        @pos = start
        @length = length
    end

    def each
        shot = Replay::ReplayShot.new
        shot.source = @source
        length = @length
        pos = @pos

        while length > 0
            shot.start = pos
            yield shot.preview
            length -= 1
            pos += 1
        end
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

    # Roll the first shot right now, and roll the rest after it's done.
    put '/roll_queue.json' do
        unless inbound_shots.empty?
            replay_app.program.shot = inbound_shots.shift
            inbound_shots.each do |shot|
                replay_app.program.queue_shot(shot)
            end
        end

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
        shot = Replay::ReplayShot.new
        shot.source = Object.from_persist_id(params[:id].to_i)
        shot.start = params[:timecode].to_i
        render :jpg => shot.preview
    end

    get '/sources/:id/:start/:length/video.mjpg' do
        source = Object.from_persist_id(params[:id].to_i)
        start = params[:start].to_i
        length = params[:length].to_i

        render :mjpg => MjpegIterator.new(source, start, length)
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

    def inbound_shots
        unless params[:inbound_shots]
            inp = environment['rack.input']
            inp.rewind
            shots_json = JSON.parse(inp.read)
            params[:inbound_shots] = shots_json.map { |json| Replay::ReplayShot.from_json(json) }
        end

        params[:inbound_shots]
    end
end

server = ReplayServer.new
control = ReplayLocalControl.new(app)
control.shot_target = server
server.replay_app = app
Thread.new { server.run(:Host => '::0', :Port => 3000) }
Thin::Logging.debug = true
control.start_irb

