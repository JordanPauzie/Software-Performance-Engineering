trap 'kill $BGPID; exit' INT
cd ../player
make -B LOCAL=1
cd ../lcgui/packages/server
npm run dev &
BGPID=$!
cd ../client
npm run dev