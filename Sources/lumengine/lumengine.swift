// The Swift Programming Language
// https://docs.swift.org/swift-book

import cxxLumengine

public final class Lumengine {
    public let empty: Bool

    public init() {
        let buffer = Buffer()
        empty = buffer.empty()
        print("Lumengine initialized")
    }

    public func hello() -> String {
        "Hello World?? - \(empty)"
    }
}
