# Instrucciones de compilacion + ejecucion

## 1. En primer lugar vamos a importar las librerias Spyne y Zeep para el servicio web de Python:
    ### $ chmod +x pip_install.sh
    ### $ ./pip_install.sh

## 2. Compilamos el servidor y el servidor RPC
    ### $ make ó $ make -f Makefile
    ### $ make -f Makefile.message_rpc
    *** Atencion! Ignorar el fichero rpc_management_client.c y rpc_management_client. Por motivos de tiempo no hemos
    podido eliminarlos del Makefile pero no son necesarios en nada ***
    *** En caso de hacer $ make -f clean Makefile.message_rpc, se dispone de backups_rpc, directorio que contiene la recuperacion
        rpc_management_server.c, Makefile.rpc_management y rpc_management.x ***

## 3. Abrir, como minimo cuatro terminales para las ejecuciones:
    ### Primera ventana:
        $ ./servidor (puerto), ej: $ ./servidor 5500
    ### Segunda ventana:
        $ ./rpc_management_server
    ### Tercera ventana:
        $ python3 webservice.py -p [puerto] o $ python3 webservice.py --p [puerto] o $ python3 webservice.py (para puerto defualt 8000)
        Ej: $ python3 webservice.py -p 3030 / $ python3 webservice.py --p 3030 / $ python3 webservice.py
    ### Cuarta ventana (o mas si se quiere)
        $ python3 client.py -s (IP) -p (puerto) -wsp (puerto de web service)
        Ej: $ python3 client.py -s 127.0.0.1 -p 5500 -wsp 8080

## 4. Indispensable! Cada cliente debe tener junto a su .py, un directorio llamado ficheros_locales/ donde almacene su contenido