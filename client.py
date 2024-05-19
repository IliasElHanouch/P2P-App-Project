from enum import Enum
import argparse
import socket
import threading
import time
import os
from zeep import Client
class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None # Direccion de servidor inicializada a none
    _port = -1 # Direccion de puerto inicializada a -1
    _socket_comunicacion = None # Descriptor de socket para este cliente inicializado a none inicialmente, antes de conectarse
    _connected = None


    # ******************** METHODS *******************

    @staticmethod
    def crear_socket():
        """ Funcion de crear y devolver un descriptor de socket con el servidor y puerto pasados por parametro al inicializar el programa """
        server_address = (client._server, client._port)
        sd_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sd_client.connect(server_address)
        except Exception as e:
            return -1
        return sd_client
    
    @staticmethod
    def crear_socket_cliente(ip, port):
        """ 
        Funcion de crear y devolver un descriptor de socket con el servidor y puerto pasados por parametro para que un usuario pueda conectarse con otro

        @param1: ip -> direccion IP del cliente2 para que el cliente1 pueda establecer conexion con el 
        @param2: puerto -> numero de puerto del cliente2 para que el cliente1 pueda establecer conexion con el 
        """
        server_address = (ip, port)
        sd_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sd_client.connect(server_address)
        except Exception as e:
            raise e
        return sd_client
    
    @staticmethod
    def get_datetime_from_web_service(port):
        try:
            client = Client(f'http://localhost:{port}/?wsdl')            
            response = client.service.get_datetime()
            return response
        except Exception as e:
            print("Exception:", str(e))
            return None  
        
    @staticmethod
    def register(user):
        """ 
        Funcion de registro para un username. Este manda solicitud de REGISTER al servidor y se registra con su nombre de usuario

        @param1: user -> nombre de usuario (en string) 
        """
        if len(user) > 256:
            print("c> REGISTER FAIL") # Longitud mayor al maximo de username
            return
        try:
            # Intentar establecer conexion con el socket para el servidor
            current_datetime = client.get_datetime_from_web_service(client._web_service_port) # Fecha y hora del serv. web
            sd_client = client.crear_socket()
            # Cadenas codificadas en bytes con fin de cadena
            sd_client.sendall(b"REGISTER\0")
            sd_client.sendall(current_datetime.encode() + b"\0")
            sd_client.sendall(user.encode() + b"\0")
            # Esperar por la repsuesta op_retorno del servidor
            response = int(readLine(sd_client))
            if response == 0:
                print("c> REGISTER OK") # Todo bien
            elif response == 1:
                print("c> USERNAME IN USE") # Ya registrado
            else:
                print("c> REGISTER FAIL") # Otro fallo (puede ser codigo 2)
        except Exception as e:
            raise e
        finally:
            sd_client.close()

    @staticmethod
    def unregister(user) :
        """ 
        Funcion de borrar registro para un username. Este manda solicitud de UNREGISTER al servidor y se registra con su nombre de usuario

        @param1: user -> nombre de usuario (en string) 
        """
        if len(user) > 256:
            print("c> UNREGISTER FAIL") # Longitud mayor al maximo de username
            return
        # Caso de error desde el cliente: evitamos borrar el registro de un usuario que este conectado por motivos de seguridad en la app
        if user == client._connected:
            print("c> UNREGISTER FAIL") # Longitud mayor al maximo de username
            return
        try:
            current_datetime = client.get_datetime_from_web_service(client._web_service_port)
            # Enviar cadena + usuario + esperar respuesta en el socket que se intenta crear
            sd_client = client.crear_socket()
            sd_client.sendall(b"UNREGISTER\0")
            sd_client.sendall(current_datetime.encode() + b"\0")
            sd_client.sendall(user.encode() + b"\0")
            response = int(readLine(sd_client))
            if response == 0:
                print("c> UNREGISTER OK")
            elif response == 1:
                print("c> USER DOES NOT EXIST") # No estaba registrado
            else:
                print("c> UNREGISTER FAIL")
        except Exception as e:
            raise e
        finally:
            sd_client.close()

    @staticmethod
    def connect(user):
        """ 
        Funcion de conexion para un username que se asume que estaregistrado. Este manda solicitud de CONNECT al servidor y
        se conecta con su nombre de usuario registrado

        @param1: user -> nombre de usuario (en string) 
        """
        if len(user) > 256:
            print("c> CONNECT FAIL") # Longitud mayor al maximo de username
            return
        # Comprobar que el cliente actual no tiene estatus de conectado
        if client._connected:
            print("c> USER ALREADY CONNECTED")
            return
        # Le buscamos un puerto cualquiera disponible PARA ESUCHAR PETICIONES(con el bind a 0)
        client._socket_comunicacion = socket.socket()
        client._socket_comunicacion.bind(('', 0))
        port = client._socket_comunicacion.getsockname()[1]
        # Crear el hilo que escuche por las peticiones de los demas clientes mientras este activa la conexion y
        # recibe el socket que le hemos creado para esta labor y que sera permanente para toda la conexion
        client_thread = threading.Thread(target=client.listen_clients, args=(client._socket_comunicacion,))
        try:
            current_datetime = client.get_datetime_from_web_service(client._web_service_port)
            # Se le envia CONNECT + username + puerto en formato bytes + se espera op_retorno
            sd_client = client.crear_socket()
            sd_client.sendall(b"CONNECT\0")
            sd_client.sendall(current_datetime.encode() + b"\0")
            sd_client.sendall(user.encode() + b"\0")
            sd_client.sendall(str(port).encode() + b"\0")
            response = int(readLine(sd_client))
            if response == 0:
                print("c> CONNECT OK")
                # Adquiere el estatus conectado con el nombre de ese usuario
                client._connected = user
                # Ahora que todo ha ido correctamente, ya puedo inicializar el hilo
                client_thread.start()
            elif response == 1:
                print("c> CONNECT FAIL, USER DOES NOT EXIST") # No esta registrado siquiera
            elif response == 2:
                # Ya esta conectado (alguna sesion anterior no cerrada correctamente u otro cliente ya conectado con ese nombre)
                print("c> USER ALREADY CONNECTED")
            else:
                print("c> CONNECT FAIL") # Cualquier otro fallo
        except Exception as e:
            raise e
        finally:
            sd_client.close()

    @staticmethod
    def listen_clients(sd):
        """
        Funcion de hilo para nuestro cliente que escucha las peticiones de los demas clientes (en este caso, GET_FILE unicamente)

        @param1: sd -> descriptor de socket para el cliente que escucha las peticiones entrantes
        """
        try:
            # Ejecuta un bucle indefinido al igual que el servidor, esperando aceptar solicitudes entrantes de otros clientes
            sd.listen(3)
            while True:
                # Aceptar la conexion para el socket, esperando la operacion GET_FILE
                sd_client, addr = sd.accept()
                tipo_operacion = readLine(sd_client)
                if tipo_operacion == "GET_FILE":
                    remote_filename = readLine(sd_client)
                    try:
                        # Como se establece que cada cliente tiene el fichero real en una estructura de directorio 'ficheros_locales'
                        # se intenta abrir el solicitado por el usuario para lectura binaria
                        with open(os.path.join('ficheros_locales', remote_filename), 'rb') as file:
                            content = file.read() # Copiar los datos
                        # Se envia codigo 0 de exito para que el cliente sepa que esta todo listo para la transferencia y luego enviarle el contenido
                        # para que lo pueda escribir en el fichero de destino
                        sd_client.sendall(b"0\0")
                        sd_client.sendall(content)
                    except FileNotFoundError:
                        sd_client.sendall(b"1\0")
                sd_client.close()
                
        except Exception as e:
            # Si ocurriese cualquier excepcion (generalmente la que origina shutdown para despertar en la desconexion al hilo), se termina esta funcion
            return

    @staticmethod
    def disconnect(user) :
        """ 
        Funcion de desconexion para un username que se asume que esta registrado y conectado. Este manda solicitud de DISCONNECT al servidor y
        se desconecta con su nombre de usuario proporcionado

        @param1: user -> nombre de usuario (en string) 
        """
        # Doble comprobacion: el lado cliente asegura que el usuario no este desconectado en la sesion actual
        if client._connected == None:
            print("c> DISCONNECT FAIL / USER NOT CONNECTED")
            return
        if client._connected != user:
            print("c> DISCONNECT FAIL")
            return
        try:
            current_datetime = client.get_datetime_from_web_service(client._web_service_port)
            sd_client = client.crear_socket()
            sd_client.sendall(b"DISCONNECT\0")
            sd_client.sendall(current_datetime.encode() + b"\0")
            sd_client.sendall(user.encode() + b"\0")
            # Casos de respuesta por parte del servidor
            response = int(readLine(sd_client))
            if response == 0:
                print("c> DISCONNECT OK")
                # En primer lugar despertamos y cerramos el hilo que esta recibiendo peticiones para ese usuario generando una excepcion en el
                client._socket_comunicacion.shutdown(socket.SHUT_RDWR)
                client._socket_comunicacion.close()
                client._socket_comunicacion = None # Eliminamos toda referencia del socket
                client._connected = None # Desreferenciar usuario con estatus conectado, esta libre de conectarse con otro registered username
            elif response == 1:
                print("c> DISCONNECT FAIL / USER DOES NOT EXIST")
            elif response == 2:
                print("c> DISCONNECT FAIL / USER NOT CONNECTED")
            else:
                print("c> DISCONNECT FAIL")
        except Exception as e:
            raise e
        finally:
            sd_client.close()

    @staticmethod
    def publish(fileName,  description):
        """ 
        Funcion de publicacion de fichero para un username que se asume que esta registrado y conectado. Este manda solicitud de PUBLISH al servidor y
        publica con el username de user._connected (nombre de usuario de la sesion actual, asignado una vez conectado), y junto a el, el nombre
        del fichero (no su contenido como tal), y una breve descripcion para dicho fichero

        @param1: fileName -> nombre de fichero (en string)
        @param2: description -> descripcion de fichero (en string)  
        """
        # Caso de error: nombre de usuario con espacios, o longitud de filename mayor a 256 o descripcion igual
        if ' ' in fileName or len(fileName) > 256 or len(description) > 256:
            print("c> PUBLISH FAIL")
            return
        # Doble comprobacion desde cliente: usuario no conectado
        if not client._connected:
            print("c> PUBLISH FAIL, USER NOT CONNECTED")
            return
        try:
            current_datetime = client.get_datetime_from_web_service(client._web_service_port)
            sd_client = client.crear_socket()
            sd_client.sendall(b"PUBLISH\0")
            sd_client.sendall(current_datetime.encode() + b"\0")
            sd_client.sendall(client._connected.encode() + b"\0")
            sd_client.sendall(fileName.encode() + b"\0")
            sd_client.sendall(description.encode() + b"\0")
            response = int(readLine(sd_client))
            if response == 0:
                print("c> PUBLISH OK")
            elif response == 1:
                print("c> PUBLISH FAIL , USER DOES NOT EXIST")
            elif response == 2:
                print("c> PUBLISH FAIL , USER NOT CONNECTED")
            elif response == 3:
                print("c> PUBLISH FAIL , CONTENT ALREADY PUBLISHED")
            else:
                print("c> PUBLISH FAIL") # Codigo 4 o cualquier otro error
        except Exception as e:
            raise e
        finally:
            sd_client.close()

    @staticmethod
    def delete(fileName) :
        """
        Funcion de borrado de fichero para un username que se asume que esta registrado y conectado. Este manda solicitud de DELETE al servidor y
        borra con el username de user._connected (nombre de usuario de la sesion actual, asignado una vez conectado), y junto a el, el nombre
        del fichero a borrar

        @param1: fileName -> nombre de fichero (en string)
        """
        # Caso de error: nombre de usuario con espacios, o longitud de filename mayor a 256
        if ' ' in fileName or len(fileName) > 256:
            print("c> DELETE FAIL")
            return
        # Doble comprobacion desde cliente: usuario no conectado
        if not client._connected:
            print("c> DELETE FAIL , USER NOT CONNECTED")
            return
        try:
            current_datetime = client.get_datetime_from_web_service(client._web_service_port)
            sd_client = client.crear_socket()
            sd_client.sendall(b"DELETE\0")
            sd_client.sendall(current_datetime.encode() + b"\0")
            sd_client.sendall(client._connected.encode() + b"\0")
            sd_client.sendall(fileName.encode() + b"\0")
            response = int(readLine(sd_client))
            if response == 0:
                print("c> DELETE OK")
            elif response == 1:
                print("c> DELETE FAIL , USER DOES NOT EXIST")
            elif response == 2:
                print("c> DELETE FAIL , USER NOT CONNECTED")
            elif response == 3:
                print("c> DELETE FAIL , CONTENT NOT PUBLISHED")
            else:
                print("c> DELETE FAIL") # Codigo 4 o cualquier otro error
        except Exception as e:
            raise e
        finally:
            sd_client.close()

    @staticmethod
    def listusers():
        """
        Funcion de listado de usuarios solicitada por un username que se asume que esta registrado y conectado. Este manda solicitud de LIST_USERS
        al servidor y lista con el username de user._connected (nombre de usuario de la sesion actual, asignado una vez conectado)
        """
        # Doble comprobacion desde cliente: usuario no conectado
        if not client._connected:
            print("c> LIST_USERS FAIL \ USER NOT CONECTED")
            return
        try:
            current_datetime = client.get_datetime_from_web_service(client._web_service_port)
            sd_client = client.crear_socket()
            sd_client.sendall(b"LIST_USERS\n")
            sd_client.sendall(current_datetime.encode() + b"\0")
            sd_client.sendall(client._connected.encode() + b"\0")
            # Inicializar una lista para almacenar la información de cada usuario
            users_information_list = []
            # Primero recibimos la operacion de retorno para comprobar que todo esta OK
            op_retorno = readLine(sd_client)
            if int(op_retorno) == 0:
                num_usuarios = readLine(sd_client)
                # Recibir toda la respuesta del servidor
                for _ in range(int(num_usuarios)):
                    # Leer la lInea de informaciOn del usuario Y hacer pushback en la lista
                    user_info = readLine(sd_client)
                    users_information_list.append(user_info)
                # Mensaje de que todo ha ido bien y mostrar info por pantalla
                print("c> LIST_USERS OK")
                for elemento in users_information_list:
                    print("\t{}".format(elemento))

            else:
                print("c> LIST_USERS FAIL")
        except Exception as e:
            raise e
        finally:
            sd_client.close()

    @staticmethod
    def listcontent(user) :
        """
        Funcion de listado de contenido solicitada por un username que se asume que esta registrado y conectado, del contenido de otro usuario registrado.
        Este manda solicitud de LIST_CONTENT al servidor y lista con el username de user._connected (nombre de usuario de la sesion actual, asignado una vez conectado)
        el contenido remoto de user del parametro de la funcion (se asume que es un usuario que debe estar registrado al menos)

        @param1: user -> nombre de usuario remoto
        """
        # Comprobar que la longitud del username es correcta como siempre
        if len(user) > 256:
            print("c> LIST_CONTENT FAIL")
            return
        # Doble comprobacion desde cliente: usuario no conectado
        if not client._connected:
            print("c> LIST_CONTENT FAIL, USER NOT CONNECTED")
            return
        try:
            current_datetime = client.get_datetime_from_web_service(client._web_service_port)
            sd_client = client.crear_socket()
            sd_client.sendall(b"LIST_CONTENT\n")
            sd_client.sendall(current_datetime.encode() + b"\0")
            sd_client.sendall(client._connected.encode() + b"\0")
            sd_client.sendall(user.encode() + b"\0")
            # Inicializar una lista para almacenar la información de cada usuario
            users_information_list = []
            op_retorno = readLine(sd_client)
            if int(op_retorno) == 0:
                num_usuarios = readLine(sd_client)
                # Recibir toda la respuesta del servidor
                for i in range(int(num_usuarios)):
                    # Leer la línea de información del usuario
                    user_info = readLine(sd_client)
                    # Agregar la información a la lista de usuarios
                    users_information_list.append(user_info)
                print("c> LIST_CONTENT OK")
                for elemento in users_information_list:
                    print("\t{}".format(elemento))
            elif int(op_retorno) == 1:
                print("c> LIST_CONTENT FAIL , USER DOES NOT EXIST")
            elif int(op_retorno) == 2:
                print("c> LIST_CONTENT FAIL , USER NOT CONNECTED")
            elif int(op_retorno) == 3:
                print("c> LIST_CONTENT FAIL , REMOTE USER DOES NOT EXIST")
            else:
                print("c> LIST_CONTENT FAIL")
        except Exception as e:
            raise e
        finally:
            sd_client.close()

    @staticmethod
    def getfile(user, remote_filename, local_filename):
        """
        Funcion de transferencia de archivos entre dos usuarios emparejados. En primer lugar, se va a solicitar al servidor la IP y el puerto
        que el usuario ve listando a los usuarios, del cliente remoto al que se va a enviar la peticion. Una vez obtenidos esos datos, va a solicitar
        con un socket emparejado al destino, el fichero remoto del que quiere el contenido, para copiarlo dentro de su directorio local en otro fichero

        @param1: user -> usuario de destino con el que se quiere emparejar
        @param1: remote_filename -> fichero remoto de user del cual se quiere obtener su contenido
        @param1: local_filename -> fichero del cliente donde se quiere copiar el contenido obtenido del usuario remoto
        """
        if len(user) > 256:
            print("c> LIST_CONTENT FAIL")
            return
        # Doble comprobacion desde cliente: usuario no conectado
        if not client._connected:
            print("c> LIST_CONTENT FAIL, USER NOT CONNECTED")
            return
        try:
            # Antes que establecer el peer con el otro cliente del que queremos su fichero, vamos a intentar obtener su puerto e IP
            # para logicamente establecer conexion con el, pidiendole esta informacion al servidor
            sd_client_server = client.crear_socket()
            sd_client_server.sendall(b"GET_FILE\0")
            sd_client_server.sendall(user.encode() + b"\0")
            sd_client_server.sendall(remote_filename.encode() + b"\0")
            # Si por lo que sea no se ha podido obtener informacion para emparejarse con el
            response_server = int(readLine(sd_client_server))
            if response_server == 1:
                print("c> GET_FILE FAIL / FILE NOT EXIST")
                return -1
            if response_server == 2:
                print("c> GET_FILE FAIL")
                return -1
            # Comprobar que IP y puerto son correctos
            ip = readLine(sd_client_server)
            port = int(readLine(sd_client_server))
            if (not port) or (not ip):
                print("c> GET_FILE FAIL")
                return -1
            # Creamos un socket emparejado con el cliente del que queremos obtener su informacion y le mandamos el mismo codigo de GET_FILE y el archivo a obtener
            sd_client = client.crear_socket_cliente(ip, int(port))
            sd_client.sendall(b"GET_FILE\0")
            sd_client.sendall(remote_filename.encode() + b"\0")
            # Espera la respuesta del servidor
            response = int(readLine(sd_client))
            # Si todo ha ido bien, vamos a copiar el contenido recibido en el fichero local
            if response == 0:
                print("c> GET_FILE OK")
                # Vamos a abrir el fichero para escritura truncada en modo binario, y asi no exigimos al usuario que el fichero destino este ya creado
                with open(os.path.join('ficheros_locales', local_filename), 'wb') as file:
                    while True:
                        # Leemos y escribimos datos hasta que se deje de recibir contenido en el flujo de transferencia
                        content_chunk = sd_client.recv(4096)
                        if not content_chunk:
                            break
                        file.write(content_chunk)
            # Si no se ha podido obtener dicha informacion, es que el fichero que queremos no existe
            elif response == 1:
                print("c> GET_FILE FAIL / FILE NOT EXIST")
            else:
                print("c> GET_FILE FAIL") # Cualquier otro error

        except Exception as e:
            raise e
        
        finally:
            # Cerrar tanto el socket del servidor como el socket con el que nos hemos comunicado con el otro cliente
            if sd_client is not None and sd_client_server is not None:
                sd_client_server.close()
                sd_client.close()


    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True) :
            try :
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER") :
                        if (len(line) == 2) :
                            client.register(line[1])
                        else :
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0]=="UNREGISTER") :
                        if (len(line) == 2) :
                            client.unregister(line[1])
                        else :
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0]=="CONNECT") :
                        if (len(line) == 2) :
                            client.connect(line[1])
                        else :
                            print("Syntax error. Usage: CONNECT <userName>")
                    
                    elif(line[0]=="PUBLISH") :
                        if (len(line) >= 3) :
                            #  Remove first two words
                            description = ' '.join(line[2:])
                            client.publish(line[1], description)
                        else :
                            print("Syntax error. Usage: PUBLISH <fileName> <description>")

                    elif(line[0]=="DELETE") :
                        if (len(line) == 2) :
                            client.delete(line[1])
                        else :
                            print("Syntax error. Usage: DELETE <fileName>")

                    elif(line[0]=="LIST_USERS") :
                        if (len(line) == 1) :
                            client.listusers()
                        else :
                            print("Syntax error. Use: LIST_USERS")

                    elif(line[0]=="LIST_CONTENT") :
                        if (len(line) == 2) :
                            client.listcontent(line[1])
                        else :
                            print("Syntax error. Usage: LIST_CONTENT <userName>")

                    elif(line[0]=="DISCONNECT") :
                        if (len(line) == 2) :
                            client.disconnect(line[1])
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="GET_FILE") :
                        if (len(line) == 4) :
                            client.getfile(line[1],line[2], line[3])
                        else :
                            print("Syntax error. Usage: GET_FILE <userName> <remote_fileName> <local_fileName>")

                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            if client._connected:
                                client.disconnect(client._connected)
                            break
                        else :
                            print("Syntax error. Use: QUIT")
                    else :
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception: " + str(e))

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")


    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def  parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        parser.add_argument('-wsp', type=int, required=True, help='Web Serice Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)) or (((args.wsp < 1024) or (args.wsp > 65535))):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535");
            return False;
        if (args.wsp == args.p):
            parser.error("Error: server Port and Web Service port must be different")
            return False
        
        client._server = args.s
        client._port = args.p
        client._web_service_port = args.wsp

        return True


    # ******************** MAIN *********************
    @staticmethod
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return

        #  Write code here
        client.shell()
        print("+++ FINISHED +++")
    

def readLine(sock):
    message = ""
    while True:
        chunk = sock.recv(1)
        if chunk == b'\0':
            return message
        message += chunk.decode()
if __name__=="__main__":
    client.main([])
