//
//  Copyright (c) 2022 Open Whisper Systems. All rights reserved.
//

import Foundation

public extension Array where Element == String? {
    func toMaybeStrings() -> [SSKMaybeString] {
        return map {
            if let value = $0 {
                return value as NSString
            }
            return NSNull()
        }
    }
}

public extension Array where Element == SSKMaybeString {
    var sequenceWithNils: AnySequence<String?> {
        return AnySequence(lazy.map { $0.stringOrNil })
    }
}

public extension Array {

    /// Returns an array of only non-nil elements.
    func compacted<T>() -> [T] where Element == T? {
        return self.compactMap({ $0 })
    }
}
