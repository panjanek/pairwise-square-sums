

def log2tsv(in_fns, out_fn):
    with open(out_fn, 'w') as out_file:
        for in_fn in in_fns:
            with open(in_fn) as in_file:
                for line in in_file:
                    if 'found 5' in line and '[' in line and ']' in line:
                        numbers = line[line.index('['):(line.index(']')+1)]
                        print(numbers)
                        txt = numbers.replace('[','').replace(']','\n').replace(',','\t')
                        out_file.write(txt)
                        
    
    
    
    
log2tsv(['../results/log509mln.txt', '../results/log500-1mld.txt', '../results/log1mld-100.txt', '../results/log1100mln-500.txt'], '../results/1mld500.tsv')