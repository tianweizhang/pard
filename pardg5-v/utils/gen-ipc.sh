echo "Guest#0,Guest#1,Guest#2,Guest#3" > guest-ipc.csv
grep 'system.switch_cpus0.ipc' stats.txt | awk '{print $2}' > guest-ipc-0
grep 'system.switch_cpus1.ipc' stats.txt | awk '{print $2}' > guest-ipc-1
grep 'system.switch_cpus2.ipc' stats.txt | awk '{print $2}' > guest-ipc-2
grep 'system.switch_cpus3.ipc' stats.txt | awk '{print $2}' > guest-ipc-3
paste guest-ipc-0 guest-ipc-1 guest-ipc-2 guest-ipc-3 | sed 's/\t/,/g' >> guest-ipc.csv
