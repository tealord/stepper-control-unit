<?php
	include 'settings.php';
	if (isset($_POST['left'])) {
		shell_exec('printf "sid=0;cmd=set;direction=0;\r" > ' . $SERIAL_PORT);
		shell_exec('printf "sid=0;cmd=set;mode=0;\r" > ' . $SERIAL_PORT);
	}
	if (isset($_POST['stop'])) {
		shell_exec('printf "sid=0;cmd=set;mode=1;\r" > ' . $SERIAL_PORT);
	}
	if (isset($_POST['right'])) {
		shell_exec('printf "sid=0;cmd=set;direction=1;\r" > ' . $SERIAL_PORT);
		shell_exec('printf "sid=0;cmd=set;mode=0;\r" > ' . $SERIAL_PORT);
	}
	if (isset($_POST['set-startpoint'])) {
		shell_exec('printf "sid=0;cmd=set;startpoint;\r" > ' . $SERIAL_PORT);
	}
	if (isset($_POST['set-endpoint'])) {
		shell_exec('printf "sid=0;cmd=set;endpoint;\r" > ' . $SERIAL_PORT);
	}
	if (isset($_POST['travel'])) {
		shell_exec('printf "sid=0;cmd=set;mode=2;\r" > ' . $SERIAL_PORT);
	}
	if (isset($_POST['exposure_time_ms'])) {
		shell_exec('printf "sid=0;cmd=set;exposure_time_ms=4000;\r" > ' . $SERIAL_PORT);
	}
	if (isset($_POST['exposure_time_offset_ms'])) {
		shell_exec('printf "sid=0;cmd=set;exposure_time_offset_ms=2000;\r" > ' . $SERIAL_PORT);
	}
	if (isset($_POST['sms_pictures_amount'])) {
		shell_exec('printf "sid=0;cmd=set;sms_pictures_amount=5;\r" > ' . $SERIAL_PORT);
	}
	if (isset($_POST['runSMS'])) {
		shell_exec('printf "sid=0;cmd=set;runSMS=1;\r" > ' . $SERIAL_PORT);
	}
?>

<!DOCTYPE html>
<html>
	<head>
		<title>Stepper Control Unit</title>
	</head>
	<body>
		<h1>Stepper Control Unit</h1>
		<h2>Basic Controls</h2>
		<form action="/index.php" method="post">
			<input type="submit" name="left" value="Left">
			<input type="submit" name="stop" value="Stop">
			<input type="submit" name="right" value="Right">
			<br>
			<input type="submit" name="set-startpoint" value="Set Startpoint">
			<input type="submit" name="set-endpoint" value="Set Endpoint">
			<br>
			<input type="submit" name="travel" value="Start Travel">
		</form> 
		<h2>Shoot Move Shoot</h2>
		<form action="/index.php" method="post">
			<input type="submit" name="exposure_time_ms" value="Exposure 4000ms"><br>
			<input type="submit" name="exposure_time_offset_ms" value="Offset 2000ms"><br>
			<input type="submit" name="sms_pictures_amount" value="Pictures 5"><br>
			<input type="submit" name="runSMS" value="Start"><br>
		</form>
	</body>
</html>
