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
            buf_size = opts[:buf_size] || 20.gigabytes
            frame_size = opts[:frame_size] || 512.kilobytes
            input = opts[:input] || fail("Cannot have a source with no input")
            file = opts[:file] || fail("Cannot have a source with no file")
            name = opts[:name] || file

            @buffer = ReplayBuffer.new(file, buf_size, frame_size, name)
            @ingest = ReplayIngest.new(input, @buffer)
        end

        def make_shot_now
            @buffer.make_shot(0, ReplayBuffer::END)
        end

        def monitor
            @ingest.monitor
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
            Replay::create_decklink_output_adapter(0, 0, RawFrame::CbYCrY8422)
        end
    end

    class ReplayShot
        def preview
            @@previewer ||= ReplayFrameExtractor.new
            @@previewer.extract_raw_jpeg(self, 0)
        end

        def make_json
            {
                :source => source.persist_id,
                :source_name => source.name,
                :start => start,
                :length => length
            }
        end     

        def self.from_json(json)
            # check if our JSON is hash-like already, if so don't parse again
            if json.respond_to? :each_pair
                data = json
            else
                data = JSON.parse(json)
            end

            shot = self.new

            p data
            shot.source = Object.from_persist_id(data["source"])
            shot.start = data["start"] if data["start"]
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
        end

        def add_source(opts={})
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

        def preview
            @preview
        end

        def program
            @program
        end

        def source(i)
            @sources[i]
        end

        def each_source
            if block_given?
                @sources.each { yield }
            else 
                @sources.each
            end
        end

        def start_irb
            IRB.start_session(binding())
        end
    end
end
