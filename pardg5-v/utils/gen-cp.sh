echo "Guest#0,Guest#1,Guest#2,Guest#3" > guest-cp.csv
grep 'system.l2.tags.guestCapacity::Guest#0' stats.txt | awk '{print $2}' > guest-cp-0
grep 'system.l2.tags.guestCapacity::Guest#1' stats.txt | awk '{print $2}' > guest-cp-1
grep 'system.l2.tags.guestCapacity::Guest#2' stats.txt | awk '{print $2}' > guest-cp-2
grep 'system.l2.tags.guestCapacity::Guest#3' stats.txt | awk '{print $2}' > guest-cp-3
paste guest-cp-0 guest-cp-1 guest-cp-2 guest-cp-3 | sed 's/\t/,/g' >> guest-cp.csv
