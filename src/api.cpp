#include <bits/stdc++.h>

int main(){
    IOReactor ioc;
    httpserver server(ioc, address, port);

    server.listen([&ioc](clientconnection){
        clientconnection.url;
        clientconnection.headers;
        clientconnection.body;
        httpconnection apiconnection(ioc, address, port);
        apiconnection.send(data, [&ioc, &clientconnection](apiconnection){
            apiconnection.status
            apiconnection.headers
            apiconnection.body
            dbconnection dbconnection(ioc, address, port);
            query.send(data, [&ioc, &clientconnection](dbconnection){
                dbconnection.status
                dbconnection.data
                clientconnection.send(data, [&ioc, &clientconnection](clientconnection){
                    clientconnection.close();
                });
            });
        });
    });

}

/*OBJECTS*/
httprequest
httpresponse
dbrequest
dbresponse


/*CLASSES*/
httpserver
clientconnection
apiconnection
dbconnection

class server{
    ioc;
    socket;
        /*
            get a new socket
            bind and listen on that socket
            register a callback on ioc
            accept and get new socket descriptor
            create a new clientconnection object with new socket descriptor
            register new socket descriptor to ioc for read or disconnect
            pass the socket descriptor to user defined callback
        */
        
    
}

class clientconnection{
    ioc;
    socket;
    httprequest;
    httpresponse;
        /*
            methods:
                read request data : returns fully formed http request struct
                write response data : pass fully formed http response struct

            on each event on the socket:
                -check the event type
                -pass the data to llhttp
                -after parsing complete invoke user defined callback with complete data
        */
};

class apiconnection{
    ioc;
    socket;
    httprequest;
    httpresponse;
    /*
        methods:
            write request data : pass fully formed http request struct
            read response data : returns fully formed http response struct

        on each event on the socket:
            -check the event type
            -pass the data to the llhttp
            -after parsing complete invoke user defined callback with complete data
            -close the connection
    */
};

class dbconnection{
    ioc;
    socket;
    dbrequest;
    dbresponse
    /*
        methods:
            write request data : pass fully formed db query struct
            read response data : pass fully formed db result struct

        on each event on the socket:
            -check the event type
            -pass the data to the libpq
            -after reading all results invoke user defined callback with complete data
            -close the connection
    */
}