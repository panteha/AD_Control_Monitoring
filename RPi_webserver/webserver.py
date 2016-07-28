# This program creates a local web server on the RPI and logs the data to a SD card and plots the data using plotly library.
# To create the database go to http://rpi_ip_address:5000/create_table
# To view the plots of both sensors (temperature and ultrasonic) go to http://rpi_ip_address:5000/
# For more information contact Panteha Ahmadi.

from flask import Flask, g, make_response
import sqlite3
import time
import datetime

app = Flask(__name__)

def get_db():
	if getattr(g, 'db', None) is None:
		g.db = sqlite3.connect('sensors.sqlite3')
	return g.db

@app.teardown_appcontext
def close_database(exception):
	if getattr(g, 'db', None) is not None:
		g.db.close()


@app.route('/create_table')
def creat_table():
	db = get_db()
	cur = db.cursor()
	cur.execute('''CREATE TABLE sensorData (ID INTEGER PRIMARY KEY AUTOINCREMENT, date_stamp TEXT, sensor_type TEXT, reading REAL )''')
	cur.close()
	db.commit()
	return 'Table created'

@app.route('/record_sensor/<sensor_type>/<float:sensor_reading>')
def sensor_read(sensor_reading, sensor_type):
	db = get_db()
	cur = db.cursor()
	date_stamp = str(datetime.datetime.fromtimestamp(int(time.time())).strftime('%Y-%m-%d %H:%M:%S.000'))
	cur.execute('INSERT INTO sensorData (date_stamp,sensor_type,reading) VALUES(?,?,?)', [(date_stamp),(sensor_type),(sensor_reading)])
	cur.close()
	db.commit()
	return 'Date/Time: <b>%s</b> <p>Sensor type: <b>%s</b></p> <p>Sensor value: <b>%f</b></p>' % (date_stamp,sensor_type,sensor_reading)


@app.route('/')
def make_plots():
	return '''
	<head>
  <script src="/static/plotly-latest.min.js"></script>
  <META HTTP-EQUIV="refresh" CONTENT="5*30">
</head>

<body>
  
  <div id="plottemperature" style="float: left; width: 50%; height: 600px;"></div>
  <div id="plotultrasonic" style="float: left; width: 50%; height: 600px;"></div>
  <script src="/plot.js/temperature"></script>
  <script src="/plot.js/ultrasonic"></script>
</body>'''


@app.route('/plot.js/<sensor_type>')
def plot_js(sensor_type):

	db = get_db()
	cur = db.cursor()
	dbrows = cur.execute('SELECT date_stamp, reading FROM sensorData WHERE sensor_type = ? ORDER BY datetime(date_stamp) DESC LIMIT 5000',[(sensor_type)] )

	rows = list(dbrows)
	cur.close()

	ylabel = ''
	if sensor_type == 'temperature':
		ylabel = 'Temperature (C)'
	else:
		ylabel = 'Distance (cm)'

 	output = '''
 	var trace1 = {
 	x:['''
	for row in rows[:-1]:
		output += '"'
		output += str(row[0])
		output += '",'
	output += '"'
	output += str(rows[-1][0])
	output += '"],\ny: ['
	for row in rows[:-1]:
		output += str(row[1])
		output += ','
	output += str(rows[-1][1])
	output += '''], 
  type: 'scatter'
};

var data = [trace1];
  Plotly.newPlot('plot%s', data, {title: "Plot of %s", xaxis: {title: "Date/Time"}, yaxis: {title: "%s"}});''' % (sensor_type, sensor_type, ylabel)
	return output

if __name__ == '__main__':
    from tornado.wsgi import WSGIContainer
    from tornado.httpserver import HTTPServer
    from tornado.ioloop import IOLoop

    http_server = HTTPServer(WSGIContainer(app))
    http_server.listen(5000)
    IOLoop.instance().start()
