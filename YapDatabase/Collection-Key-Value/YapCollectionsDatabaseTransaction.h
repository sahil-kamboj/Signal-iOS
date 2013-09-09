#import <Foundation/Foundation.h>
#import "YapAbstractDatabaseTransaction.h"

@class YapCollectionsDatabaseConnection;

/**
 * Welcome to YapDatabase!
 *
 * The project page has a wealth of documentation if you have any questions.
 * https://github.com/yaptv/YapDatabase
 *
 * If you're new to the project you may want to visit the wiki.
 * https://github.com/yaptv/YapDatabase/wiki
 * 
 * Transactions represent atomic access to a database.
 * There are two types of transactions:
 * - Read-Only transactions
 * - Read-Write transactions
 *
 * Once a transaction is started, all data access within the transaction from that point forward until completion
 * represents an atomic "snapshot" of the current state of the database. For example, if a read-write operation
 * occurs in parallel with a read-only transaction, the read-only transaction won't see the changes made by
 * the read-write operation. But once the read-write operation completes, all transactions started from that point
 * forward will see the changes.
 *
 * You first create and configure a YapCollectionsDatabase instance.
 * Then you can spawn one or more connections to the database file.
 * Each connection allows you to execute transactions in a serial fashion.
 * For concurrent access, you can create multiple connections,
 * and execute transactions on each connection simulataneously.
 *
 * Concurrency is straight-forward. Here are the rules:
 *
 * - You can have multiple connections.
 * - Every connection is thread-safe.
 * - You can have multiple read-only transactions simultaneously without blocking.
 *   (Each simultaneous transaction would be going through a separate connection.)
 * - You can have multiple read-only transactions and a single read-write transaction simultaneously without blocking.
 *   (Each simultaneous transaction would be going through a separate connection.)
 * - There can only be a single transaction per connection at a time.
 *   (Transactions go through a per-connection serial queue.)
 * - There can only be a single read-write transaction at a time.
 *   (Read-write transactions go through a per-database serial queue.)
**/

/**
 * A YapCollectionsDatabaseReadTransaction encompasses a single read-only database transaction.
 * You can execute multiple operations within a single transaction.
 *
 * A transaction allows you to safely access the database as needed in a thread-safe and optimized manner.
**/
@interface YapCollectionsDatabaseReadTransaction : YapAbstractDatabaseTransaction

/**
 * Transactions are light-weight objects created by connections.
 *
 * Connections are the parent objects of transactions.
 * Connections own the transaction objects.
 *
 * Transactions store nearly all their state in the parent connection object.
 * This reduces the memory requirements for transactions objects,
 * and reduces the overhead associated in creating them.
**/
@property (nonatomic, unsafe_unretained, readonly) YapCollectionsDatabaseConnection *connection;

#pragma mark Count

/**
 * Returns the total number of collections.
 * Each collection may have 1 or more key/object pairs.
**/
- (NSUInteger)numberOfCollections;

/**
 * Returns the total number of keys in the given collection.
 * Returns zero if the collection doesn't exist (or all key/object pairs from the collection have been removed).
**/
- (NSUInteger)numberOfKeysInCollection:(NSString *)collection;

/**
 * Returns the total number of key/object pairs in the entire database (including all collections).
**/
- (NSUInteger)numberOfKeysInAllCollections;

#pragma mark List

/**
 * Returns a list of all collection names.
**/
- (NSArray *)allCollections;

/**
 * Returns a list of all keys in the given collection.
**/
- (NSArray *)allKeysInCollection:(NSString *)collection;

#pragma mark Primitive

/**
 * Primitive access.
 *
 * These are available for in-case you store irregular data
 * that shouldn't go through configured serializer/deserializer.
 *
 * @see objectForKey:inCollection:
 * @see metadataForKey:inCollection:
**/
- (NSData *)primitiveDataForKey:(NSString *)key inCollection:(NSString *)collection;
- (NSData *)primitiveMetadataForKey:(NSString *)key inCollection:(NSString *)collection;
- (BOOL)getPrimitiveData:(NSData **)dataPtr
       primitiveMetadata:(NSData **)primitiveMetadataPtr
                  forKey:(NSString *)key
            inCollection:(NSString *)collection;

#pragma mark Object

/**
 * Object access.
 * Objects are automatically deserialized using database's configured deserializer.
**/
- (id)objectForKey:(NSString *)key inCollection:(NSString *)collection;

/**
 * Returns whether or not the given key/collection exists in the database.
**/
- (BOOL)hasObjectForKey:(NSString *)key inCollection:(NSString *)collection;

/**
 * Provides access to both object and metadata in a single call.
 *
 * @return YES if the key exists in the database. NO otherwise, in which case both object and metadata will be nil.
**/
- (BOOL)getObject:(id *)objectPtr metadata:(id *)metadataPtr forKey:(NSString *)key inCollection:(NSString *)collection;

#pragma mark Metadata

/**
 * Provides access to the metadata.
 * This fetches directly from the metadata dictionary stored in memory, and thus never hits the disk.
**/
- (id)metadataForKey:(NSString *)key inCollection:(NSString *)collection;

#pragma mark Enumerate

/**
 * Fast enumeration over all keys in the given collection.
 *
 * This uses a "SELECT key FROM database WHERE collection = ?" operation,
 * and then steps over the results invoking the given block handler.
**/
- (void)enumerateKeysInCollection:(NSString *)collection
                       usingBlock:(void (^)(NSString *key, BOOL *stop))block;

/**
 * Fast enumeration over all keys in the given collection.
 *
 * This uses a "SELECT collection, key FROM database" operation,
 * and then steps over the results invoking the given block handler.
**/
- (void)enumerateKeysInAllCollectionsUsingBlock:(void (^)(NSString *collection, NSString *key, BOOL *stop))block;

/**
 * Fast enumeration over all keys and associated metadata in the given collection.
 * 
 * This uses a "SELECT key, metadata FROM database WHERE collection = ?" operation and steps over the results.
 * 
 * If you only need to enumerate over certain items (e.g. keys with a particular prefix),
 * consider using the alternative version below which provides a filter,
 * allowing you to skip the deserialization step for those items you're not interested in.
 * 
 * Keep in mind that you cannot modify the collection mid-enumeration (just like any other kind of enumeration).
**/
- (void)enumerateKeysAndMetadataInCollection:(NSString *)collection
                                  usingBlock:(void (^)(NSString *key, id metadata, BOOL *stop))block;

/**
 * Fast enumeration over all keys and associated metadata in the given collection.
 *
 * From the filter block, simply return YES if you'd like the block handler to be invoked for the given key.
 * If the filter block returns NO, then the block handler is skipped for the given key,
 * which avoids the cost associated with deserializing the object.
 * 
 * Keep in mind that you cannot modify the collection mid-enumeration (just like any other kind of enumeration).
**/
- (void)enumerateKeysAndMetadataInCollection:(NSString *)collection
                                  usingBlock:(void (^)(NSString *key, id metadata, BOOL *stop))block
                                  withFilter:(BOOL (^)(NSString *key))filter;



/**
 * Fast enumeration over all key/metadata pairs in all collections.
 * 
 * This uses a "SELECT metadata FROM database ORDER BY collection ASC" operation, and steps over the results.
 * 
 * If you only need to enumerate over certain objects (e.g. keys with a particular prefix),
 * consider using the alternative version below which provides a filter,
 * allowing you to skip the deserialization step for those objects you're not interested in.
 * 
 * Keep in mind that you cannot modify the database mid-enumeration (just like any other kind of enumeration).
**/
- (void)enumerateKeysAndMetadataInAllCollectionsUsingBlock:
                                        (void (^)(NSString *collection, NSString *key, id metadata, BOOL *stop))block;

/**
 * Fast enumeration over all key/metadata pairs in all collections.
 *
 * This uses a "SELECT metadata FROM database ORDER BY collection ASC" operation and steps over the results.
 * 
 * From the filter block, simply return YES if you'd like the block handler to be invoked for the given key.
 * If the filter block returns NO, then the block handler is skipped for the given key,
 * which avoids the cost associated with deserializing the object.
 *
 * Keep in mind that you cannot modify the database mid-enumeration (just like any other kind of enumeration).
 **/
- (void)enumerateKeysAndMetadataInAllCollectionsUsingBlock:
                                        (void (^)(NSString *collection, NSString *key, id metadata, BOOL *stop))block
                             withFilter:(BOOL (^)(NSString *collection, NSString *key))filter;

/**
 * Fast enumeration over all objects in the database.
 *
 * This uses a "SELECT key, object from database WHERE collection = ?" operation, and then steps over the results,
 * deserializing each object, and then invoking the given block handler.
 *
 * If you only need to enumerate over certain objects (e.g. keys with a particular prefix),
 * consider using the alternative version below which provides a filter,
 * allowing you to skip the serialization step for those objects you're not interested in.
**/
- (void)enumerateKeysAndObjectsInCollection:(NSString *)collection
                                 usingBlock:(void (^)(NSString *key, id object, BOOL *stop))block;

/**
 * Fast enumeration over objects in the database for which you're interested in.
 * The filter block allows you to decide which objects you're interested in.
 *
 * From the filter block, simply return YES if you'd like the block handler to be invoked for the given key.
 * If the filter block returns NO, then the block handler is skipped for the given key,
 * which avoids the cost associated with deserializing the object.
**/
- (void)enumerateKeysAndObjectsInCollection:(NSString *)collection
                                 usingBlock:(void (^)(NSString *key, id object, BOOL *stop))block
                                 withFilter:(BOOL (^)(NSString *key))filter;

/**
 * Enumerates all key/object pairs in all collections.
 * 
 * The enumeration is sorted by collection. That is, it will enumerate fully over a single collection
 * before moving onto another collection.
 * 
 * If you only need to enumerate over certain objects (e.g. subset of collections, or keys with a particular prefix),
 * consider using the alternative version below which provides a filter,
 * allowing you to skip the serialization step for those objects you're not interested in.
**/
- (void)enumerateKeysAndObjectsInAllCollectionsUsingBlock:
                                            (void (^)(NSString *collection, NSString *key, id object, BOOL *stop))block;

/**
 * Enumerates all key/object pairs in all collections.
 * The filter block allows you to decide which objects you're interested in.
 *
 * The enumeration is sorted by collection. That is, it will enumerate fully over a single collection
 * before moving onto another collection.
 * 
 * From the filter block, simply return YES if you'd like the block handler to be invoked for the given
 * collection/key pair. If the filter block returns NO, then the block handler is skipped for the given pair,
 * which avoids the cost associated with deserializing the object.
**/
- (void)enumerateKeysAndObjectsInAllCollectionsUsingBlock:
                                            (void (^)(NSString *collection, NSString *key, id object, BOOL *stop))block
                                 withFilter:(BOOL (^)(NSString *collection, NSString *key))filter;

/**
 * Fast enumeration over all rows in the database.
 *
 * This uses a "SELECT key, data, metadata from database WHERE collection = ?" operation,
 * and then steps over the results, deserializing each object & metadata, and then invoking the given block handler.
 *
 * If you only need to enumerate over certain rows (e.g. keys with a particular prefix),
 * consider using the alternative version below which provides a filter,
 * allowing you to skip the serialization step for those rows you're not interested in.
**/
- (void)enumerateRowsInCollection:(NSString *)collection
                       usingBlock:(void (^)(NSString *key, id object, id metadata, BOOL *stop))block;

/**
 * Fast enumeration over rows in the database for which you're interested in.
 * The filter block allows you to decide which rows you're interested in.
 *
 * From the filter block, simply return YES if you'd like the block handler to be invoked for the given key.
 * If the filter block returns NO, then the block handler is skipped for the given key,
 * which avoids the cost associated with deserializing the object & metadata.
**/
- (void)enumerateRowsInCollection:(NSString *)collection
                       usingBlock:(void (^)(NSString *key, id object, id metadata, BOOL *stop))block
                       withFilter:(BOOL (^)(NSString *key))filter;

/**
 * Enumerates all rows in all collections.
 * 
 * The enumeration is sorted by collection. That is, it will enumerate fully over a single collection
 * before moving onto another collection.
 * 
 * If you only need to enumerate over certain rows (e.g. subset of collections, or keys with a particular prefix),
 * consider using the alternative version below which provides a filter,
 * allowing you to skip the serialization step for those objects you're not interested in.
**/
- (void)enumerateRowsInAllCollectionsUsingBlock:
                            (void (^)(NSString *collection, NSString *key, id object, id metadata, BOOL *stop))block;

/**
 * Enumerates all rows in all collections.
 * The filter block allows you to decide which objects you're interested in.
 *
 * The enumeration is sorted by collection. That is, it will enumerate fully over a single collection
 * before moving onto another collection.
 * 
 * From the filter block, simply return YES if you'd like the block handler to be invoked for the given
 * collection/key pair. If the filter block returns NO, then the block handler is skipped for the given pair,
 * which avoids the cost associated with deserializing the object.
**/
- (void)enumerateRowsInAllCollectionsUsingBlock:
                            (void (^)(NSString *collection, NSString *key, id object, id metadata, BOOL *stop))block
                 withFilter:(BOOL (^)(NSString *collection, NSString *key))filter;

/**
 * Enumerates over the given list of keys (unordered).
 *
 * This method is faster than fetching individual items as it optimizes cache access.
 * That is, it will first enumerate over items in the cache and then fetch items from the database,
 * thus optimizing the cache and reducing query size.
 *
 * If any keys are missing from the database, the 'metadata' parameter will be nil.
 *
 * IMPORTANT:
 * Due to cache optimizations, the items may not be enumerated in the same order as the 'keys' parameter.
**/
- (void)enumerateMetadataForKeys:(NSArray *)keys
                    inCollection:(NSString *)collection
             unorderedUsingBlock:(void (^)(NSUInteger keyIndex, id metadata, BOOL *stop))block;

/**
 * Enumerates over the given list of keys (unordered).
 *
 * This method is faster than fetching individual items as it optimizes cache access.
 * That is, it will first enumerate over items in the cache and then fetch items from the database,
 * thus optimizing the cache and reducing query size.
 *
 * If any keys are missing from the database, the 'object' parameter will be nil.
 *
 * IMPORTANT:
 * Due to cache optimizations, the items may not be enumerated in the same order as the 'keys' parameter.
**/
- (void)enumerateObjectsForKeys:(NSArray *)keys
                   inCollection:(NSString *)collection
            unorderedUsingBlock:(void (^)(NSUInteger keyIndex, id object, BOOL *stop))block;

/**
 * Enumerates over the given list of keys (unordered).
 *
 * This method is faster than fetching individual items as it optimizes cache access.
 * That is, it will first enumerate over items in the cache and then fetch items from the database,
 * thus optimizing the cache and reducing query size.
 *
 * If any keys are missing from the database, the 'object' and 'metadata' parameter will be nil.
 *
 * IMPORTANT:
 * Due to cache optimizations, the items may not be enumerated in the same order as the 'keys' parameter.
**/
- (void)enumerateRowsForKeys:(NSArray *)keys
                inCollection:(NSString *)collection
         unorderedUsingBlock:(void (^)(NSUInteger keyIndex, id object, id metadata, BOOL *stop))block;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark -
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

@interface YapCollectionsDatabaseReadWriteTransaction : YapCollectionsDatabaseReadTransaction

/* Inherited from YapAbstractDatabaseTransaction

- (void)rollback;

*/

#pragma mark Primitive

/**
 * Primitive access.
 *
 * These are available in-case you store irregular data
 * that shouldn't go through configured serializer/deserializer.
 *
 * @see objectForKey:collection:
**/
- (void)setPrimitiveData:(NSData *)data forKey:(NSString *)key inCollection:(NSString *)collection;
- (void)setPrimitiveData:(NSData *)data
                  forKey:(NSString *)key
            inCollection:(NSString *)collection
   withPrimitiveMetadata:(NSData *)primitiveMetadata;
- (void)setPrimitiveMetadata:(NSData *)primitiveMetadata forKey:(NSString *)key inCollection:(NSString *)collection;

#pragma mark Object

/**
 * Sets the object for the given key/collection.
 * Objects are automatically serialized using the database's configured serializer.
 * 
 * You may optionally pass metadata about the object.
 * The metadata is also written to the database for persistent storage, and thus persists between sessions.
 * Metadata is serialized/deserialized to/from disk just like the object.
**/
- (void)setObject:(id)object forKey:(NSString *)key inCollection:(NSString *)collection;
- (void)setObject:(id)object forKey:(NSString *)key inCollection:(NSString *)collection withMetadata:(id)metadata;

#pragma mark Metadata

/**
 * Updates the metadata, and only the metadata, for the given key/collection.
 * The object for the key doesn't change.
 *
 * Note: If there is no stored object for the given key/collection, this method does nothing.
 * If you pass nil for the metadata, any given metadata associated with the key/colleciton is removed.
**/
- (void)setMetadata:(id)metadata forKey:(NSString *)key inCollection:(NSString *)collection;

#pragma mark Remove

/**
 * Deletes the database row with the given key/collection.
 * This method is automatically called if you invoke
 * setObject:forKey:collection: or setPrimitiveData:forKey:collection: and pass nil object/data.
**/
- (void)removeObjectForKey:(NSString *)key inCollection:(NSString *)collection;

/**
 * Deletes the database rows with the given keys in the given collection.
**/
- (void)removeObjectsForKeys:(NSArray *)keys inCollection:(NSString *)collection;

/**
 * Deletes every key/object pair from the given collection.
 * No trace of the collection will remain afterwards.
**/
- (void)removeAllObjectsInCollection:(NSString *)collection;

/**
 * Removes every key/object pair in the entire database (from all collections).
**/
- (void)removeAllObjectsInAllCollections;

@end
