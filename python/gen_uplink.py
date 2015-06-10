import matplotlib.pyplot as plt
import numpy
import sys # cmd line arguments
import pandas


if len(sys.argv) != 2:
	raise Exception('Missing csv file name.')

csv_file_path = sys.argv[1]
results = pandas.read_csv(csv_file_path)

y_snr = [float(i) for i in results.snr]
y_pkt_loss = (numpy.array([(int(results.msgs_per_setting[0]) - int(i)) for i in results['pkt_count']]) / float(results.msgs_per_setting[0])) * 100.0
y_time = [int(i) for i in results['avg_time']]

f, (snr_ax, pkt_loss_ax, time_ax) = plt.subplots(3, sharex=True)
f.subplots_adjust(bottom=0.15)

if results.test_type[0] == 0: # POW test
	test_title = 'TX Power test'
	x = [int(i) for i in results['pow']]
	plt.xlabel("TX Power (in dB)")


elif results.test_type[0] == 1: # BW test
	test_title = 'Bandwith test'
	labels = ['BW125', 'BW250']
	x = range(1, 2+1)
	plt.xticks(x, labels, rotation='horizontal')
	plt.xlabel("Bandwith (in kHz")


elif results.test_type[0] == 2: # SF test
	test_title = 'Spreading Factor test'
	labels = ['SF7', 'SF8', 'SF9', 'SF10', 'SF11', 'SF12']
	x = range(1, 6+1)
	plt.xticks(x, labels, rotation='horizontal')
	plt.xlabel("Spreading Factor")

elif results.test_type[0] == 3: # CRC test
	test_title = 'CRC Coding test'
	labels = ['4/5', '2/3', '4/7', '1/2']
	x = range(1, 4+1)
	plt.xticks(x, labels, rotation='horizontal')
	plt.xlabel("CRC coding")


elif results.test_type[0] == 4: # SIZE test
	test_title = 'Packet Size test'
	x = [int(i) for i in results['size']]
	plt.xlabel("Packet size (in bytes)")

else:
	print "Invalid test type received."

f.suptitle(test_title, fontsize=16, fontweight='bold')

snr_ax.set_title("Signal-to-noise ratio")
snr_ax.plot(x, y_snr, '-ro')
snr_ax.set_ylabel("SNR (in dB)")

#snr_ax.vlines(x, [-4.0 ], y_snr,  linestyles='dashed')

pkt_loss_ax.set_title("Packets lost")
pkt_loss_ax.plot(x, y_pkt_loss, '-ro')
pkt_loss_ax.set_ylabel("Packet loss (%)")

time_ax.set_title("Average transmission time")
time_ax.plot(x, y_time, '-ro')
time_ax.set_ylabel("Average tx time (in ms)")

plt.figtext(0.5, 0.05, 'Fixed parameters', horizontalalignment='center', fontsize=15)

#plt.axis([0, 20, -3, 10])

plt.show()