require 'patchbay'
require_relative 'replay'
require 'json'

include Replay

class RolloutServer < Patchbay
    self.files_dir = "public_html"

    def initialize(program)
        @program = program
        @ROLLOUT_DIR = '/home/rpitv/rollout'
    end

    get '/files.json' do
        render :json => Dir.glob(@ROLLOUT_DIR + '/*.{avi,mov,mpg}').to_json
    end

    get '/fileinfo.json' do
        filelist = Dir.glob(@ROLLOUT_DIR + '/*.{avi,mov,mpg}')
        result = filelist.map do |f|
            get_file_data(f)
        end
        render :json => result.to_json
    end

    put '/ffmpeg_rollout.json' do
        p inbound_json
        filename = inbound_json["filename"]
        @program.lavf_playout(filename)
        render :json => ''
    end

    get '/rollstate.json' do
        position = @program.source_position
        duration = @program.source_duration
        data = { 'position' => position, 'duration' => duration}
        render :json => data.to_json
    end

    put '/roll_queue.json' do
        p inbound_json
        @program.lavf_playout_list(inbound_json)
        render :json => ''
    end

private
    def inbound_json
        unless params[:inbound_json]
            inp = environment['rack.input']
            inp.rewind
            params[:inbound_json] = JSON.parse(inp.read)
        end

        params[:inbound_json]
    end

    def get_file_data(f)
        duration = ReplayPlayoutLavfSource.get_file_duration(f)
        { 
            "path" => f, 
            "basename" => File.basename(f), 
            "duration" => duration 
        }
    end
end

oadp = Replay::create_decklink_output_adapter_with_audio(0, 0, RawFrame::CbYCrY8422)
prog = ReplayPlayout.new(oadp)
app = RolloutServer.new(prog)
app.run(:Host => '::0', :Port => 3000)

