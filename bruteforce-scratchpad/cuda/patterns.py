

    
sets = read_sets('out.tsv')
hist = {}
for s in sets:
    for n in s:
        if n in hist:
            hist[n] = hist[n]+1
        else:
            hist[n] = 1
        
hist2 = []
for k in hist:
    hist2.append([k,hist[k]])
   
hist2.sort(key=lambda x: x[1], reverse=False)   
print(hist2)