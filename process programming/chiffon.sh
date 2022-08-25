#!/bin/bash
if [ "${1}" == "-m" ]; then #host
n_hosts=${2}
elif [ "${1}" == "-n" ]; then #player
n_players=${2}
elif [ "${1}" == "-l" ]; then #locky number
lucky_number=${2}
fi

if [ "${3}" == "-m" ]; then #host
n_hosts=${4}
elif [ "${3}" == "-n" ]; then #player
n_players=${4}
elif [ "${3}" == "-l" ]; then #locky number
lucky_number=${4}
fi

if [ "${5}" == "-m" ]; then #host
n_hosts=${6}
elif [ "${5}" == "-n" ]; then #player
n_players=${6}
elif [ "${5}" == "-l" ]; then #locky number
lucky_number=${6}
fi
#echo "${n_hosts} ${n_players} ${lucky_number}"

# Prepare FIFO files for all M hosts.
[ -p fifo_0.tmp ] || mkfifo fifo_0.tmp #read for host_n
exec 3<> fifo_0.tmp # open for read write on fd3
for (( i=1; i<=n_hosts; i=i+1))
do
[ -p fifo_${i}.tmp ] || mkfifo  fifo_${i}.tmp #write to host_n
host[${i}]=0 # 0 for idle
fdd=$((3+${i}))
eval exec "${fdd}"'<> fifo_${i}.tmp' # open for read write on fd3
# fork hosts
./host -m ${i} -d 0 -l ${lucky_number} &
pids[${i}]=$!
#echo "pid: ${pids[${i}]}"
done

# players' score
for (( i=1; i<=n_players; i=i+1))
do
play_score[${i}]=0
play_seq[${i}]=${i}
done

game=0
game_end=0
# choose 8 player to a group
for (( l_one=1; l_one<=n_players-7; l_one=l_one+1)) 
do
for (( l_two=l_one+1; l_two<=n_players-6; l_two=l_two+1)) 
do
for (( l_thr=l_two+1; l_thr<=n_players-5; l_thr=l_thr+1)) 
do
for (( l_for=l_thr+1; l_for<=n_players-4; l_for=l_for+1)) 
do
for (( l_fiv=l_for+1; l_fiv<=n_players-3; l_fiv=l_fiv+1)) 
do
for (( l_six=l_fiv+1; l_six<=n_players-2; l_six=l_six+1)) 
do
for (( l_sev=l_six+1; l_sev<=n_players-1; l_sev=l_sev+1)) 
do
for (( l_eig=l_sev+1; l_eig<=n_players; l_eig=l_eig+1)) 
do
find=0
game=$((${game}+1))
while (( find == 0 )) # find an idle host
do

for (( h=1; h<=n_hosts; h=h+1))
do
if (( host[${h}] == 0 )); then
echo -e "${l_one} ${l_two} ${l_thr} ${l_for} ${l_fiv} ${l_six} ${l_sev} ${l_eig}\n" > fifo_${h}.tmp
#read line <fifo_${h}.tmp
#echo $line
find=1
host[${h}]=1
break
fi
done

if ((find == 0)); then
#read the result from fifo_0.tmp
    line1=$'\0\0\0\0\0'
    read line1 <fifo_0.tmp
    game_end=$((${game_end}+1))
    #echo "$line1"
#if (( ${#line1} <= 2 )); then
    host[$line1]=0
for (( i=1; i<=8; i=i+1))
do
    line1=$'\0\0\0\0\0'
    read line1 <fifo_0.tmp
    #echo "$line1"
    IFS=' ' read -ra var <<< "$line1"
    #echo "var1: ${var[0]}"
    #echo "var2: ${var[1]}"
    play_score[${var[0]}]=$((${play_score[${var[0]}]}+${var[1]}))
done
fi

done
done
done
done
done
done
done
done
done

#read the remainding
#echo "game: ${game}"
#echo "game_end: ${game_end}"
while [ "${game}" != "${game_end}" ]
do
        line1=$'\0\0\0\0\0'
        read line1 <fifo_0.tmp
        #echo "$line1"
        game_end=$((${game_end}+1))
    for (( i=1; i<=8; i=i+1))
    do
        line1=$'\0\0\0\0\0'
        read line1 <fifo_0.tmp
        #echo "$line1"
        IFS=' ' read -ra var <<< "$line1"
        #echo "var1: ${var[0]}"
        #echo "var2: ${var[1]}"
        play_score[${var[0]}]=$((${play_score[${var[0]}]}+${var[1]}))
    done
done

#send an ending message to all hosts forked.
for (( h=1; h<=n_hosts; h=h+1))
do
echo -e "-1 -1 -1 -1 -1 -1 -1 -1\n" > fifo_${h}.tmp
done

#print the final rankings ordered
#output score
#for (( i=1; i<=n_players; i=i+1))
#do
#echo "${i} ${play_score[${i}]}"
#done

#bubble sort
for (( i=1; i<=n_players; i=i+1))
do
for (( j=1; j<=n_players-i; j=j+1))
do
if (( play_score[${j}] < play_score[$((${j}+1))] ));then
tmp=${play_score[${j}]}
play_score[${j}]=${play_score[$((${j}+1))]}
play_score[$((${j}+1))]=${tmp}

tmp=${play_seq[${j}]}
play_seq[${j}]=${play_seq[$((${j}+1))]}
play_seq[$((${j}+1))]=${tmp}
fi
done
done

#calculate rank
for (( i=1; i<=n_players; i=i+1))
do
rank[${i}]=0
done

rank_num=1
acc=0
rank[${play_seq[1]}]=${rank_num}
for (( i=2; i<=n_players; i=i+1))
do
if (( play_score[${i}] < play_score[$((${i}-1))] ));then
rank_num=$((${rank_num}+1+${acc}))
acc=0
else
acc=$((1+${acc}))
fi
rank[${play_seq[${i}]}]=${rank_num}
done

#output rank
for (( i=1; i<=n_players; i=i+1))
do
echo "${i} ${rank[${i}]}"
done

# close fifo
exec 3>&-
for (( i=1; i<=n_hosts; i=i+1))
do
fdd=$((3+${i}))
eval exec "${fdd}"'>&-'
done

#remove fifo
rm fifo_0.tmp 
for (( i=1; i<=n_hosts; i=i+1))
do
rm fifo_${i}.tmp 
done

#wait for all forked processes to exit
for pid in ${pids[*]}; do
wait $pid
done
