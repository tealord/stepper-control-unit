SERIAL_PORT=	/dev/cu.usbserial-14310

all:
	echo "<?php \$$SERIAL_PORT = \"${SERIAL_PORT}\"; ?>" > settings.php
	stty -f ${SERIAL_PORT} speed 9600
	open http://localhost:8080
	sh -c "trap 'kill %1' SIGINT; cat ${SERIAL_PORT} & php -S localhost:8080 index.php"
