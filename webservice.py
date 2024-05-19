from wsgiref.simple_server import make_server 
from spyne import Application, rpc, ServiceBase, Unicode 
from spyne.protocol.soap import Soap11 
from spyne.server.wsgi import WsgiApplication 
from datetime import datetime 
import argparse

class MyService(ServiceBase):
    # Metodo RPC para obtener fecha y hora actuales
    @rpc(_returns=Unicode) 
    def get_datetime(ctx):
        now = datetime.now()
        formatted_datetime = now.strftime("%d/%m/%Y %H:%M:%S")
        return formatted_datetime

def main(port):
    # Creamos la app SOAP
    application = Application([MyService], tns='datetime_service',
                              in_protocol=Soap11(validator='lxml'),
                              out_protocol=Soap11()) 
    # Adaptar a Wsgi
    wsgi_application = WsgiApplication(application)
    # Crear servidor para que escuche
    server = make_server('127.0.0.1', port, wsgi_application)
    print("Escuchando en puerto", port)
    server.serve_forever()

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', type=int, default=8000, help='Puerto del servidor')
    args = parser.parse_args()
    return args.port 

if __name__ == '__main__':
    port = parse_arguments()
    main(port)
