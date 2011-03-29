require './keyer'
include Keyer
include Driver

keyer do |k|
    # Set up input adapter to take input from first DeckLink card
    k.input(
        decklink_input do |inp|
            inp.card_index 0
        end
    )

    # Set up output adapter to send to the second DeckLink card
    k.output(
        decklink_output do |out|
            out.card_index 1
        end
    )

    # Generate dynamic scoreboard using scoreboard_cg.rb.
    k.cg(
        svg_subprocess_cg do |cg|
            cg.command '(cd ../..; scoreboard/scoreboard_cg.rb)'
        end
    )

    # Also mix in a static SVG file.
    k.cg(
        svg_subprocess_cg do |cg|
            cg.command '(cd ../..; scoreboard/static_svg.rb)'
            cg.x 96
            cg.y 963
        end
    )
end
