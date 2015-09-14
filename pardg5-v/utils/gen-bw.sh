echo "Guest#0,Guest#1,Guest#2,Guest#3" > guest-bw.csv
grep 'system.mem_ctrls.addr_mapper.bw_total::Guest#3' stats.txt | awk '{print $2}' > guest-3                                                                   
grep 'system.mem_ctrls.addr_mapper.bw_total::Guest#2' stats.txt | awk '{print $2}' > guest-2                                                                   
grep 'system.mem_ctrls.addr_mapper.bw_total::Guest#1' stats.txt | awk '{print $2}' > guest-1
grep 'system.mem_ctrls.addr_mapper.bw_total::Guest#0' stats.txt | awk '{print $2}' > guest-0
paste guest-0 guest-1 guest-2 guest-3 | sed 's/\t/,/g' >> guest-bw.csv
