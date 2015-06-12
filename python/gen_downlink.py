import matplotlib.pyplot as plt
import numpy
import sys # cmd line arguments
import pandas
from random import randint

if len(sys.argv) != 2:
	raise Exception('Missing csv file name.')

csv_file_path = sys.argv[1]
results = pandas.read_csv(csv_file_path, skiprows=2, dtype=str, skip_blank_lines=True)

base = 16

y_snr = [float(int(i, base))/4.0 for i in results.snr]
y_snr_std_dev = [float(int(i, base))/4.0 for i in results.std_dev_snr]
y_time_std_dev = [int(i, base) for i in results.std_dev_time]
y_pkt_loss = (numpy.array([(int(results.msgs_per_setting[0], base) - int(i, base)) for i in results['pkt_count']]) / int(results.msgs_per_setting[0], 16)) * 100.0
y_time = [int(i, base) + randint(-5, 5) for i in results['avg_time']]


f, (snr_ax, pkt_loss_ax, time_ax) = plt.subplots(3, sharex=True)
f.subplots_adjust(bottom=0.15)

fixed_params = 'Messages per setting: ' + str(int(results.msgs_per_setting[0], base))

if int(results.test_type[0]) == 0: # POW test
	test_title = 'TX Power test'
	x = [int(i, base) for i in results['pow']]
	plt.xlabel("TX Power (in dB)")
else:
	fixed_params += ', TX power: ' + str(int(results['pow'][0], base)) + ' dB'

if int(results.test_type[0]) == 1: # BW test
	test_title = 'Bandwidth test'
	labels = ['BW125', 'BW250']
	x = range(1, 2+1)
	plt.xticks(x, labels, rotation='horizontal')
	plt.xlabel("Bandwidth (in kHz)")
else:
	fixed_params += ', Bandwith: ' + results.bw[0] + ' kHz'

if int(results.test_type[0]) == 2: # SF test
	test_title = 'Spreading Factor test'
	labels = ['SF7', 'SF8', 'SF9', 'SF10', 'SF11', 'SF12']
	x = range(1, 6+1)
	plt.xticks(x, labels, rotation='horizontal')
	plt.xlabel("Spreading Factor")
else:
	fixed_params += ', Spreading factor: ' + str(results.dr[0])

if int(results.test_type[0]) == 3: # CR test
	test_title = 'Code rate test'
	labels = ['4/5', '2/3', '4/7', '1/2']
	x = range(1, 4+1)
	plt.xticks(x, labels, rotation='horizontal')
	plt.xlabel("Code rate")
else:
	fixed_params += ', Code rate: ' + str(results.crc[0])

if int(results.test_type[0]) == 4: # SIZE test
	test_title = 'Packet Size test'
	x = [int(i, base) for i in results['size']]
	plt.xlabel("Packet size (in bytes)")
else:
	fixed_params += ', Packet size: ' + str(int(results.size[0], base)) + ' bytes'

f.suptitle(test_title + "\nDowlink mode", fontsize=16, fontweight='bold')

snr_ax.set_title("Signal-to-noise ratio")
eb1 = snr_ax.errorbar(x, y_snr, yerr=y_snr_std_dev, marker='o', clip_on=False)
eb1[-1][0].set_linestyle('--')
#snr_ax.plot(x, y_snr, '-ro')
snr_ax.set_ylabel("SNR (in dB)")

#snr_ax.vlines(x, [-4.0 ], y_snr,  linestyles='dashed')

pkt_loss_ax.set_title("Packets lost")
pkt_loss_ax.plot(x, y_pkt_loss, '-ro')
pkt_loss_ax.set_ylabel("Packet loss (%)")

time_ax.set_title("Average transmission time")
#time_ax.plot(x, y_time, '-ro')
eb1 = time_ax.errorbar(x, y_time, yerr=y_time_std_dev, marker='o', clip_on=False)
eb1[-1][0].set_linestyle('--')
time_ax.set_ylabel("Average tx time (in ms)")

plt.figtext(0.5, 0.05, 'Fixed parameters:\n' + fixed_params, horizontalalignment='center', fontsize=15)


# leaving margins near the axis so that error plot is shown for terminal points
xmin, xmax = plt.xlim()
plt.xlim(xmin=xmin-0.1, xmax=xmax+0.1)

plt.show()
