import cxxConfig

public final class OnConnectHandler {
    public let closure: (OpaquePointer, std.error_code) -> TCPCommandVariant?

    public init(closure: @escaping (OpaquePointer, std.error_code) -> TCPCommandVariant?) {
        self.closure = closure
    }

    public func run(session: TcpSessionPtr, ec: std.error_code) -> TCPCommandVariant? { 
        guard let pointer = get_pointer_from(session) else {
            return nil
        }

        return closure(pointer, ec)
    }
}
