require 'rubygems'
require_relative 'replay'

class Integer
    def gigabytes
        self * 1024 * 1024 * 1024
    end

    def megabytes
        self * 1024 * 1024
    end

    def kilobytes
        self * 1024
    end
end

module Replay
    class ReplaySource
        def initialize(opts={})
            input = opts[:input]
            mjpeg_cmd = opts[:mjpeg_cmd]
            file = opts[:file] || fail("Cannot have a source with no file")
            name = opts[:name] || file
            game_data = opts[:game_data] || \
                fail("Cannot create source without game data");

            @buffer = ReplayBuffer.new(file, name)

            if input
                @ingest = ReplayIngest.new(input, @buffer, game_data)
            elsif mjpeg_cmd
                @ingest = ReplayMjpegIngest.new(mjpeg_cmd, @buffer)
            else
                fail "need some input source"
            end
        end

        def debug
            @ingest.debug
        end

        def suspend_encode
            @ingest.suspend_encode
        end

        def resume_encode
            @ingest.resume_encode
        end

        def make_shot_now
            @ingest.trigger     # trigger buffer dump from remote camera
            @buffer.make_shot(0, ReplayBuffer::END)
        end

        def make_shot_at(timecode)
            @buffer.make_shot(timecode, ReplayBuffer::START)
        end

        def monitor
            @ingest.monitor
        end

        def buffer
            @buffer
        end
    end

    class ReplayPreview
        def shot
            # workaround for swig not handling output params properly
            sh = ReplayShot.new
            get_shot(sh)
            sh
        end
    end

    class ReplayMultiviewer
        # throw a Ruby-ish frontend on the ugly C++
        def add_source(opts={})
            params = ReplayMultiviewerSourceParams.new
            params.source = opts[:port] || fail("Need some kind of input")
            params.x = opts[:x] || 0
            params.y = opts[:y] || 0

            real_add_source(params)
        end
    end

    # abstraction for what will someday be a config file parser
    class ReplayConfig
        def make_output_adapter
            Replay::create_decklink_output_adapter_with_audio(7, 0, RawFrame::CbYCrY8422)
        end
    end

    class ReplayShot
        def preview
            @@previewer ||= ReplayFrameExtractor.new
            @@previewer.extract_raw_jpeg(self, 0)
        end

        def audio
            @@previewer ||= ReplayFrameExtractor.new
            @@previewer.extract_raw_audio(self, 0)
        end

        def thumbnail
            @@previewer ||= ReplayFrameExtractor.new
            @@previewer.extract_thumbnail_jpeg(self, 0)
        end

        def frame(x)
            @@previewer ||= ReplayFrameExtractor.new
            @@previewer.extract_raw_jpeg(self, x)
        end

        def make_json(source_id)
            {
                "source" => source_id,
                "source_name" => source.name,
                "start" => start,
                "length" => length
            }
        end     

        def self.from_json(json, sources)
            # check if our JSON is hash-like already, if so don't parse again
            if json.respond_to? :each_pair
                data = json
            else
                data = JSON.parse(json)
            end


            p data
            start = data["start"]
            shot = sources[data["source"]].make_shot_at(start)
            shot.length = data["length"] if data["length"]

            shot
        end
    end

    class ReplayApp
        def initialize
            @sources = []

            @dpys = FramebufferDisplaySurface.new
            @multiviewer = ReplayMultiviewer.new(@dpys)

            config = ReplayConfig.new # something something something

            @game_data = ReplayGameData.new
            @program = ReplayPlayout.new(config.make_output_adapter)
            @preview = ReplayPreview.new

            # wire program and preview into multiviewer
            @multiviewer.add_source(:port => @preview.monitor,
                    :x => 0, :y => 0)
            @multiviewer.add_source(:port => @program.monitor,
                    :x => 960, :y => 0)


            # FIXME hard coded defaults
            @source_pvw_width = 480
            @source_pvw_height = 270
            @mvw = 1920
            @mvx = 0
            @mvy = 540
            @filters = []
        end

        def toggle_filter(fid)
            if fid < @filters.length
                filter = @filters[fid]
                if filter.is_enabled
                    filter.disable
                else
                    filter.enable
                end
            end
        end

        def mv_mode
            @multiviewer.change_mode
        end

        def add_png_file_dsk(png, x, y)
            filter = ReplayPlayoutImageFilter.from_png(png, x, y)
            @program.register_filter(filter)
            @filters << filter
            return @filters.length - 1
        end

        def add_source(opts={})
            opts.merge!({ :game_data => @game_data })
            source = ReplaySource.new(opts)
            @sources << source

            @multiviewer.add_source(:port => source.monitor, 
                    :x => @mvx, :y => @mvy)

            @mvx += @source_pvw_width
            if @mvx >= @mvw
                @mvx = 0
                @mvy += @source_pvw_height
            end
        end


        def start
            @multiviewer.start
        end

        attr_reader :preview, :program, :game_data

        def source(i)
            @sources[i]
        end

        def sources
            @sources
        end

        def each_source
            if block_given?
                @sources.each { yield }
            else 
                @sources.each
            end
        end

        def suspend_encode
            @sources.each { |source| source.suspend_encode }    
        end

        def resume_encode
            @sources.each { |source| source.resume_encode }
        end

        def debug
            @sources.each { |source| source.debug }
        end

        def start_irb
            IRB.start_session(binding())
        end
    end
end
