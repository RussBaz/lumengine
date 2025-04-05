@testable import lumengine
import Testing

@Test func example() async throws {
    // Write your test here and use APIs like `#expect(...)` to check expected conditions.
    print("Hello World")
    let l = Lumengine()
    #expect(l.hello() == "Hello World??")
}
