import argparse
import os
import subprocess

protocol_list = ['MI', 'MSI', 'MESI', 'MOESI', 'Dragon']

def run_cmd(cmd):
    print(f"Running: {cmd}")
    out = subprocess.run(cmd, shell=True, check=True)
    return out

def output_dir(protocol):
    if(os.path.exists(protocol) == True):
        files = os.listdir(protocol)
        for file_name in files:
            file_path = os.path.join(protocol, file_name)
            os.remove(file_path)
    else :
        os.mkdir(protocol)

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--protocol', type=str, default='MSI', help='Protocol name (MI, MSI, MESI, MOESI, Dragon)')
    # parser.add_argument('--no_cores', type=int, default='4', help='No of cores can be 4 or 8')
    return parser.parse_args()

def get_config(protocol):
    if protocol == 'MI':
        return f"ex_proc_MI.config"
    elif protocol == 'MSI':
        return f"ex_proc_MSI.config"
    elif protocol == 'MESI':
        return f"ex_proc_MESI.config"
    elif protocol == 'MOESI':
        return f"ex_proc_MOESI.config"
    elif protocol == 'Dragon':
        return f"ex_proc_Dragon.config"
    elif protocol == 'all':
        return f"all"

    else:
        assert(False and "Invalid protocol")

def run_trace(protocol, config_name):
    trace_path = '/afs/cs.cmu.edu/academic/class/15346-s22/public/traces/coher/'

    trace_4 = ['blackscholes_4_simsmall.taskgraph', 'dedup_4_simsmall.taskgraph', 'fluidanimate_4_simsmall.taskgraph', 'splash2x.lu_cb_4_simsmall.taskgraph', 'splash2x.lu_ncb_4_simsmall.taskgraph', 'splash2x.volrend_4_simsmall.taskgraph', 'swaptions_4_simsmall.taskgraph']
    trace_8 = ['dedup_8_simsmall.taskgraph', 'splash2x.lu_cb_8_simsmall.taskgraph', 'splash2x.lu_ncb_8_simsmall.taskgraph', 'splash2x.volrend_8_simsmall.taskgraph', 'x264_8_simsmall.taskgraph']
    
    output_dir(protocol)

    for tr in trace_4:
        cmd = f"./cadss-engine -s {config_name} -t {trace_path}{tr} -n 4 -c coherentCache > {protocol}/{tr}.txt"
        run_cmd(cmd)

    for tr in trace_8:
        cmd = f"./cadss-engine -s {config_name} -t {trace_path}{tr} -n 8 -c coherentCache > {protocol}/{tr}.txt"
        run_cmd(cmd)

def main():
    args = parse_args()

    # get the config file for the protocol
    config_name = get_config(args.protocol)

    if(config_name == 'all'):
        for config in protocol_list:
            name = get_config(config)
            run_trace(config, name)
    else:
        run_trace(args.protocol, config_name)

    

if __name__ == '__main__':
    main()