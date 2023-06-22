
project('Snek', 'cpp',
        default_options: ['buildtype=release', 'strip=true',
                          'cpp_std=c++20', 'b_lto=true'])

server_deps = [
    dependency('threads')
]

client_deps = [
    dependency('threads'),
    dependency('sfml-audio'),
    dependency('sfml-graphics')
]

ixwebsocket_srcs = [
    'libs/ixwebsocket/IXBench.cpp',
    'libs/ixwebsocket/IXCancellationRequest.cpp',
    'libs/ixwebsocket/IXConnectionState.cpp',
    'libs/ixwebsocket/IXDNSLookup.cpp',
    'libs/ixwebsocket/IXExponentialBackoff.cpp',
    'libs/ixwebsocket/IXGetFreePort.cpp',
    'libs/ixwebsocket/IXGzipCodec.cpp',
    'libs/ixwebsocket/IXHttp.cpp',
    'libs/ixwebsocket/IXHttpClient.cpp',
    'libs/ixwebsocket/IXHttpServer.cpp',
    'libs/ixwebsocket/IXNetSystem.cpp',
    'libs/ixwebsocket/IXSelectInterrupt.cpp',
    'libs/ixwebsocket/IXSelectInterruptFactory.cpp',
    'libs/ixwebsocket/IXSelectInterruptPipe.cpp',
    'libs/ixwebsocket/IXSelectInterruptEvent.cpp',
    'libs/ixwebsocket/IXSetThreadName.cpp',
    'libs/ixwebsocket/IXSocket.cpp',
    'libs/ixwebsocket/IXSocketConnect.cpp',
    'libs/ixwebsocket/IXSocketFactory.cpp',
    'libs/ixwebsocket/IXSocketServer.cpp',
    'libs/ixwebsocket/IXSocketTLSOptions.cpp',
    'libs/ixwebsocket/IXStrCaseCompare.cpp',
    'libs/ixwebsocket/IXUdpSocket.cpp',
    'libs/ixwebsocket/IXUrlParser.cpp',
    'libs/ixwebsocket/IXUuid.cpp',
    'libs/ixwebsocket/IXUserAgent.cpp',
    'libs/ixwebsocket/IXWebSocket.cpp',
    'libs/ixwebsocket/IXWebSocketCloseConstants.cpp',
    'libs/ixwebsocket/IXWebSocketHandshake.cpp',
    'libs/ixwebsocket/IXWebSocketHttpHeaders.cpp',
    'libs/ixwebsocket/IXWebSocketPerMessageDeflate.cpp',
    'libs/ixwebsocket/IXWebSocketPerMessageDeflateCodec.cpp',
    'libs/ixwebsocket/IXWebSocketPerMessageDeflateOptions.cpp',
    'libs/ixwebsocket/IXWebSocketProxyServer.cpp',
    'libs/ixwebsocket/IXWebSocketServer.cpp',
    'libs/ixwebsocket/IXWebSocketTransport.cpp'
]

executable('SnekServer',
            'src/server/main.cpp',
            ixwebsocket_srcs,
            dependencies: server_deps,
            include_directories: ['libs','src'])

executable('SnekClient',
            'src/client/main.cpp',
            ixwebsocket_srcs,
            dependencies: client_deps,
            include_directories: ['libs','src'])