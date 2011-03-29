require './keyer'
include Keyer
include Driver

keyer do |k|
    k.input(
        decklink_input do |inp|
            inp.card_index 0
        end
    )

    k.output(
        decklink_output do |out|
            out.card_index 1
        end
    )

    k.cg(
        svg_subprocess_cg do |cg|
            cg.command '(cd ../..; scoreboard/scoreboard_cg.rb)'
        end
    )

    k.cg(
        svg_subprocess_cg do |cg|
            cg.command '(cd ../..; scoreboard/static_svg.rb)'
            cg.x 96
            cg.y 963
        end
    )
end
