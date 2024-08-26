time=0
divider=1
cd ../bin
touch tmp.txt

while (( $time < 20000000 ))
do
    echo $divider
    mpsim.x -c1 -w --is 9 --it 1 -F0,$divider 1>/dev/null 2>/dev/null
    time=$(cat stats.txt | grep "Total simulated master system cycles" | cut -d'(' -f2 | cut -d' ' -f1)
    energy=$(cat stats.txt | grep "Total:" | head -1 | awk '{print $2}')
    echo -e "$(echo "scale=2; 200/$divider" | bc -l) $energy" >> tmp.txt
    divider=$(($divider*2))
done

time=0
divider=$(($divider/4))
head -n -1 tmp.txt > tmp2.txt; mv tmp2.txt tmp.txt

while (( $time < 20000000 ))
do
    divider=$(($divider+1))
    echo $divider
    mpsim.x -c1 -w --is 9 --it 1 -F0,$divider 1>/dev/null 2>/dev/null
    time=$(cat stats.txt | grep "Total simulated master system cycles" | cut -d'(' -f2 | cut -d' ' -f1)
    energy=$(cat stats.txt | grep "Total:" | head -1 | awk '{print $2}')
    echo -e "$(echo "scale=2; 200/$divider" | bc -l) $energy" >> tmp.txt
done

head -n -1 tmp.txt ../bench/bench_is9it1_freq.txt
cat ../bench/bench_is9it1_freq.txt
