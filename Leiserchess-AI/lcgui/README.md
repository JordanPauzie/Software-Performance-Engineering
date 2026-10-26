# Running the GUI

To use the GUI, you can run `./run.sh` from this directory. The server uses your Leiserchess binary in the `player` directory, so make sure your bot is working before running the command. Then, navigate to <http://localhost:5173/> in your browser.

# Using the GUI

There are three main uses for the GUI: playing against your bot, setting up custom board positions, and analyzing recorded games.

The "Play" tab provides an interface for playing against your bot. You can set time controls or depth limits on your bot. It also records a PGN that can be pasted into the analysis tool after the game.

The "Skittles" tab provides an interface for setting up arbitrary board positions and getting the FEN string. One may find this helpful for setting up starting positions that cause bugs in their bot.

The "Analysis" tab allows you to freely play moves and have your bot analyze game positions. To start analysis, press the gray "x" in the box next to the board. As your bot searches deeper and deeper, the analysis will be updated, which will also display the principle variations that have been found.

# Board GUI Controls

To move a piece, first click on it. This will display the possible moves. Piece rotations are represented as triangles on the same square as the selected piece. To select a move, either click on the marker corresponding to the move or, if you are moving the piece to a different square, drag the piece to that square. Hovering over a potential move will display the trajectory of the laser should you select the move, for convenience.

You can resize the board by dragging the bottom right corner. In addition, you can flip the board by pressing the gray circular arrow in the top left corner.

To make annotations on the board such as drawing arrows or marking squares with circles, you can right-click or hold shift while left-clicking and drag your mouse around.

If you are playing a game or looking at an analysis, you can press the left and right arrow keys to go to the previous and next moves, respectively. On the analysis board, you can also move pieces and build a move tree with different branches.

# Caveats

The server only runs one instance of your bot. So, the server can only process one request at a time. For example, if you are running an analysis that is taking a long time, you may find that the "Import" button for the PGN hangs. Importing a PGN requires querying the bot for game logic, which cannot be done if the bot is busy computing an analysis.

In this GUI, you can only play human vs bot matches, not bot vs bot matches. The best way to gather bot vs bot matches is to run the tester in the `tester` directory (see the README there). The tester will give you PGN files that you can then paste into the analysis page.

When you paste PGNs into the analysis page, you should exclude the tags that precede the moves in brackets, along with the game result at the end (the 0-1 or 1-0).

# Acknowledgements

Thanks to lichess for inspiring much of the design for the GUI!
