import os
import xlsxwriter

output_dir = "./result/"
protocols = ["MSI", "MESI", "MOESI", "Dragon", "HybridDragon"]
metrics = ["Ticks", "BusRd", "BusWr", "BusUpd", "Shared", "Data", "Bus Requests", "Mem Transfers", "Cumulative"]

proto_dict = {}
bench = set()

def fix_cumulative(data):
    total_val = 0
    del_list = []
    for k in data.keys():
        if "Cumulative" in k:
            total_val = total_val + data[k]
            del_list.append(k)
    for x in del_list:
        data.pop(x)
            # data.pop(k)
    data["Cumulative"] = total_val
    return data

if os.path.isfile("output.xlsx"):
    os.remove("output1.xlsx")

workbook = xlsxwriter.Workbook('output1.xlsx')

def write_excel():
    for metric in metrics:
        header =[]
        metric_data = []
        header.append("Benchmarks")
        for x in protocols:
            header.append(x)
        worksheet = workbook.add_worksheet(metric)
        for b in bench:
            row = []
            row.append(b)
            for proto in protocols:
                if b in proto_dict[proto].keys():
                    row.append(proto_dict[proto][b][metric])
            metric_data.append(row)
        row = 0
        col = 0
        for h in range(len(header)):
            worksheet.write(row, h, header[h])
        row = row+1
    
        for line in metric_data:
            for h in range(len(line)):
                worksheet.write(row, h, line[h])
            row = row + 1


    
def generate_proto_dict():
    for protocol in protocols:
        subdir = os.path.join(output_dir, protocol)
        if not os.path.exists(subdir):
            os.makedirs(subdir)
        cur_proto = {}
        for filename in os.listdir(subdir):
            if filename.endswith(".txt"):
                filepath = os.path.join(subdir, filename)
                a,b = os.path.splitext(filename)
                bench.add(a)
                with open(filepath, "r") as f:
                    contents = f.readlines()
                    data = {}
                    for line in contents:
                        key, value = line.strip().split(" - ")
                        data[key] = int(value)
                    data = fix_cumulative(data)
                    cur_proto[a] = data
        # Fix Cumulative
        proto_dict[protocol] = cur_proto

def main():
    generate_proto_dict()
    write_excel()
    workbook.close()

if __name__ == "__main__":
    main()
