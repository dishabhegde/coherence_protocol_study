import argparse
import os
import subprocess
import multiprocessing
import threading

protocol_list = ['MSI', 'MESI', 'MOESI', 'Dragon']

def run_cmd(cmd):
    print(f"Running: {cmd}")
    out = subprocess.run(cmd, shell=True, check=True)
    return out

def output_dir(protocol):
    if(os.path.exists('result') == False):
        os.mkdir('result')

    res_path = os.path.join('result',protocol)
    if(os.path.exists(res_path) == True):
        files = os.listdir(res_path)
        for file_name in files:
            file_path = os.path.join(res_path, file_name)
            os.remove(file_path)
    else :
        # os.mkdir('result')
        os.mkdir(res_path)

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--protocol', type=str, default='MSI', help='Protocol name (MSI, MESI, MOESI, Dragon or all)')
    parser.add_argument('--trace_type', type=str, default='fast', help='fast or slow')
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

def run_trace(protocol, config_name, trace_type):
    trace_path = '/afs/cs.cmu.edu/academic/class/15346-s22/public/traces/coher/'

    trace_4 = ['blackscholes_4_simsmall.taskgraph', 'dedup_4_simsmall.taskgraph', 'fluidanimate_4_simsmall.taskgraph', 'splash2x.lu_cb_4_simsmall.taskgraph', 'splash2x.lu_ncb_4_simsmall.taskgraph', 'splash2x.volrend_4_simsmall.taskgraph', 'swaptions_4_simsmall.taskgraph']
    trace_8 = ['dedup_8_simsmall.taskgraph', 'splash2x.lu_cb_8_simsmall.taskgraph', 'splash2x.lu_ncb_8_simsmall.taskgraph', 'splash2x.volrend_8_simsmall.taskgraph', 'x264_8_simsmall.taskgraph']

    fast_4 = ['blackscholes_4_simsmall.taskgraph', 'dedup_4_simsmall.taskgraph', 'splash2x.volrend_4_simsmall.taskgraph', 'swaptions_4_simsmall.taskgraph']
    fast_8 = ['splash2x.volrend_8_simsmall.taskgraph', 'x264_8_simsmall.taskgraph']
    
    output_dir(protocol)

    cmd_list = []
    if(trace_type == 'fast'):
        for tr in fast_4:
            cmd = f"./cadss-engine -s {config_name} -t {trace_path}{tr} -n 4 -c coherentCache > {'result'}/{protocol}/{tr}.txt"
            # run_cmd(cmd)
            cmd_list.append(cmd)

        for tr in fast_8:
            cmd = f"./cadss-engine -s {config_name} -t {trace_path}{tr} -n 8 -c coherentCache > {'result'}/{protocol}/{tr}.txt"
            # run_cmd(cmd)
            cmd_list.append(cmd)
    else:
        for tr in trace_4:
            cmd = f"./cadss-engine -s {config_name} -t {trace_path}{tr} -n 4 -c coherentCache > {'result'}/{protocol}/{tr}.txt"
            # run_cmd(cmd)
            cmd_list.append(cmd)

        for tr in trace_8:
            cmd = f"./cadss-engine -s {config_name} -t {trace_path}{tr} -n 8 -c coherentCache > {'result'}/{protocol}/{tr}.txt"
            # run_cmd(cmd)
            cmd_list.append(cmd)

    with multiprocessing.Pool() as pool:
        results = pool.map(run_cmd, cmd_list)

def main():
    args = parse_args()
    
    trace_type = args.trace_type


    # get the config file for the protocol
    config_name = get_config(args.protocol)

    if(config_name == 'all'):
        for config in protocol_list:
            name = get_config(config)
            run_trace(config, name, trace_type)
    else:
        run_trace(args.protocol, config_name, trace_type)

# def main():
#     args = parse_args()
    
#     trace_type = args.trace_type
#     config_name = get_config(args.protocol)

#     threads = []

#     if(config_name == 'all'):
#         for config in protocol_list:
#             name = get_config(config)
#             t = threading.Thread(target=run_trace, args=(config, name, trace_type))
#             threads.append(t)
#     else:
#         t = threading.Thread(target=run_trace, args=(args.protocol, config_name, trace_type))
#         threads.append(t)

#     # Start all threads
#     for t in threads:
#         t.start()

#     # Wait for all threads to finish
#     for t in threads:
#         t.join()

if __name__ == '__main__':
    main()