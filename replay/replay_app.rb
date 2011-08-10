require 'rubygems'
require 'backports'
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
            frame_size = opts[:frame_size] || 256.kilobytes
            input = opts[:input] || fail("Cannot have a source with no input")
            file = opts[:file] || fail("Cannot have a source with no file")
            name = opts[:name] || file

            @buffer = ReplayBuffer.new(file, buf_size, frame_size, name)
            @ingest = ReplayIngest.new(input, @buffer)
        end

        def make_shot_now
            @buffer.make_shot(0, ReplayBuffer::END)
        end
    end

    class ReplayMultiviewer
        # throw a Ruby-ish frontend on the ugly C++
        def add_source(opts={})
            params = ReplayMultiviewer::SourceParams.new
            params.source = opts[:port] || fail("Need some kind of input")
            params.x = opts[:x] || 0
            params.y = opts[:y] || 0

            real_add_source(params)
        end
    end

    class ReplayConfig
        def make_output_adapter
            create_decklink_output_adapter(0, 0, RawFrame::CbYCrY8422)
        end
    end

    class UniqueList < Hash
        def initialize
            super

            @item_id = 0
        end

        def insert(item)
            self[@item_id] = item
            @item_id += 1
            @item_id - 1
        end
    end

    class ReplayApp
        def initialize
            @sources = []
            @sources_shots = []
            @shots = ShotList.new
            
            @dpys = FramebufferDisplaySurface.new
            @multiviewer = ReplayMultiviewer.new(@dpys)

            config = ReplayConfig.new # something something something

            @program = ReplayPlayout.new(config.make_output_adapter)
            @preview = ReplayPreview.new

            @preview_shot = nil
            @program_shot = nil
        end

        def add_source(opts={})
            @sources << ReplaySource.new(opts)
            @sources_shots << nil
        end

    end
end
