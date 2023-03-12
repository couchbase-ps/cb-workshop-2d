package com.cbworkshop;

import com.couchbase.client.core.error.DocumentNotFoundException;
import com.couchbase.client.java.Collection;
import com.couchbase.client.java.*;
import com.couchbase.client.java.json.JsonObject;
import com.couchbase.client.java.kv.GetResult;
import com.couchbase.client.java.kv.MutateInSpec;
import com.couchbase.client.java.query.QueryResult;
import com.couchbase.client.java.query.ReactiveQueryResult;
import com.couchbase.client.java.search.SearchQuery;
import com.couchbase.client.java.search.queries.MatchQuery;
import com.couchbase.client.java.search.result.SearchResult;
import com.couchbase.client.java.search.result.SearchRow;
import reactor.core.publisher.Flux;

import java.time.Duration;
import java.util.*;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import static com.couchbase.client.java.query.QueryOptions.queryOptions;

public class MainLab {

    public static final String CMD_QUIT = "quit";
    public static final String CMD_CREATE = "create";
    public static final String CMD_READ = "read";
    public static final String CMD_UPDATE = "update";
    public static final String CMD_SUBDOC = "subdoc";
    public static final String CMD_DELETE = "delete";
    public static final String CMD_QUERY = "query";
    public static final String CMD_QUERY_REACTIVE = "queryreactive";
    public static final String CMD_QUERY_AIRPORTS = "queryairports";
    public static final String CMD_BULK_WRITE = "bulkwrite";
    public static final String CMD_BULK_WRITE_REACTIVE = "bulkwritereactive";
    public static final String CMD_SEARCH = "search";
    public static final String SCOPE = "inventory";
    public static final String MSG = "msg";

    private static Cluster cluster = null;
    private static Scope scope;
    private static Collection msgCollection;

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        initConnection();
        welcome();
        usage();
        String cmdLn = null;
        while (!CMD_QUIT.equalsIgnoreCase(cmdLn)) {
            try {
                System.out.print("# ");
                cmdLn = scanner.nextLine();
                process(cmdLn);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        scanner.close();
    }

    private static void initConnection() {
        String clusterAddress = System.getProperty("cbworkshop.clusteraddress");
        String user = System.getProperty("cbworkshop.user");
        String password = System.getProperty("cbworkshop.password");
        String bucketName = System.getProperty("cbworkshop.bucket");

        cluster = Cluster.connect(clusterAddress,
                ClusterOptions.clusterOptions(user, password)
                        .environment(env -> env.timeoutConfig()
                                .connectTimeout(Duration.ofSeconds(15))
                                .kvTimeout(Duration.ofSeconds(15))));

        Bucket bucket = cluster.bucket(bucketName);
        scope = bucket.scope(SCOPE);
        msgCollection = scope.collection(MSG);
    }

    private static void process(String cmdLn) {
        String words[] = cmdLn.split(" ");

        switch (words[0].toLowerCase()) {
            case CMD_QUIT:
                System.out.println("bye!");
                System.exit(0);
                break;
            case CMD_CREATE:
                create(words);
                break;
            case CMD_READ:
                read(words);
                break;
            case CMD_UPDATE:
                update(words);
                break;
            case CMD_SUBDOC:
                subdoc(words);
                break;
            case CMD_DELETE:
                delete(words);
                break;
            case CMD_QUERY:
                query();
                break;
            case CMD_QUERY_REACTIVE:
                queryReactive();
                break;
            case CMD_QUERY_AIRPORTS:
                queryAirports(words);
                break;
            case CMD_BULK_WRITE:
                bulkWrite(words);
                break;
            case CMD_BULK_WRITE_REACTIVE:
                bulkWriteReactive(words);
                break;
            case CMD_SEARCH:
                search(words);
                break;
            case "":
                // do nothing
                break;
            default:
                usage();
        }
    }

    private static void create(String[] words) {
        String key = words[1];
        String from = words[2];
        String to = words[3];
        JsonObject json = JsonObject.create()
                .put("timestamp", System.currentTimeMillis())
                .put("from", from)
                .put("to", to);
        msgCollection.insert(key, json);
        //msgCollection.upsert(key, json);
        System.out.printf("Document created in collection '%s' with key: '%s'%n", MSG, key);
    }

    private static void read(String[] words) {
        String collection = words[1];
        String key = words[2];
        GetResult result = null;
        try {
            result = scope.collection(collection).get(key);
            System.out.println(result.contentAsObject().toString());
        } catch (DocumentNotFoundException e) {
            System.out.printf("Document with key: '%s' not found in collection '%s'%n", key, collection);
        }
    }

    private static void update(String[] words) {
        String key = "airline_" + words[1];
        Collection collection = scope.collection("airline");
        GetResult result = collection.get(key);
        JsonObject doc = result.contentAsObject();
        String name = doc.getString("name");
        doc.put("name", name.toUpperCase());
        collection.replace(key, doc);
        System.out.printf("Document updated in collection '%s' with key: '%s'%n", MSG, key);
    }

    private static void subdoc(String[] words) {
        String key = words[1];
        msgCollection
                .mutateIn(key, List.of(
                        MutateInSpec.replace("from", "Administrator"),
                        MutateInSpec.insert("reviewed", System.currentTimeMillis())
                ));
        System.out.printf("Sub Document updated in collection '%s' with key: '%s'%n", MSG, key);
    }

    private static void delete(String[] words) {
        String key = words[1];
        try {
            msgCollection.remove(key);
            System.out.printf("Document deleted in collection '%s' with key: '%s'%n", MSG, key);
        } catch (DocumentNotFoundException e) {
            System.out.printf("Document with key '%s' not found in collection '%s'%n", key, MSG);
        }
    }

    private static void query() {
        QueryResult queryResult = cluster.query("SELECT * FROM `travel-sample`.inventory.airline LIMIT 10");
        for (JsonObject row : queryResult.rowsAsObject()) {
            System.out.println(row.toString());
        }
    }

    private static void queryReactive() {
        cluster.reactive()
                .query("SELECT * FROM `travel-sample`.inventory.airline LIMIT 5")
                .flatMapMany(ReactiveQueryResult::rowsAsObject)
                .subscribe(row -> System.out.println(row.toString()),
                        e -> System.err.println("N1QL Error/Warning: " + e));

    }

    private static void queryAirports(String[] words) {
        String sourceairport = words[1];
        String destinationairport = words[2];
        String queryStr = "SELECT airline.name" +
                " FROM `travel-sample`.inventory.route" +
                " JOIN `travel-sample`.inventory.airline ON KEYS route.airlineid" +
                " WHERE route.sourceairport=$src\n" +
                "   AND route.destinationairport=$dst";

        JsonObject params = JsonObject.create()
                .put("src", sourceairport)
                .put("dst", destinationairport);

        QueryResult queryResult = cluster.query(queryStr, queryOptions().parameters(params));
        for (JsonObject row : queryResult.rowsAsObject()) {
            System.out.println(row.toString());
        }
    }

    private static Map<String, JsonObject> prepareMsgs(String[] words) {
        int size = Integer.parseInt(words[1]);

        System.out.println("Deleting messages ...");
        cluster.query("DELETE FROM `travel-sample`.inventory.msg");

        Map<String, JsonObject> msgs = IntStream.range(0, size)
                .boxed()
                .collect(Collectors.toMap(Object::toString, i -> JsonObject.create()
                        .put("timestamp", System.currentTimeMillis())
                        .put("from", "me")
                        .put("to", "you")));

        System.out.printf("Writing %d messages%n", msgs.size());
        return msgs;
    }

    private static void bulkWriteReactive(String[] words) {
        Map<String, JsonObject> msgs = prepareMsgs(words);
        long ini = System.currentTimeMillis();
        Flux.fromStream(msgs.entrySet().stream())
                .flatMap(msg -> msgCollection.reactive().insert(msg.getKey(), msg.getValue()))
                .blockLast();
        System.out.printf("Time elapsed %d ms%n", System.currentTimeMillis() - ini);
    }

    private static void bulkWrite(String[] words) {
        Map<String, JsonObject> msgs = prepareMsgs(words);
        long ini = System.currentTimeMillis();
        msgs.forEach(msgCollection::insert);
        System.out.printf("Time elapsed %d ms%n", System.currentTimeMillis() - ini);
    }

    private static void search(String[] words) {
        String term = words[1];
        MatchQuery fts = SearchQuery.match(term);
        SearchResult result = cluster.searchQuery("hotels", fts);
        for (SearchRow row : result.rows()) {
            System.out.println(row);
        }
    }


    private static void welcome() {
        System.out.println("Welcome to CouchbaseJavaWorkshop!");
    }

    private static void usage() {
        System.out.println("Usage options:\n");
        System.out.printf("%s [msg_key from to]\n", CMD_CREATE);
        System.out.printf("%s [collection key]\n", CMD_READ);
        System.out.printf("%s [airline_key]\n", CMD_UPDATE);
        System.out.printf("%s [msg_key]\n", CMD_SUBDOC);
        System.out.printf("%s [msg_key]\n", CMD_DELETE);
        System.out.printf("%s\n", CMD_QUERY);
        System.out.printf("%s [sourceairport destinationairport]\n", CMD_QUERY_AIRPORTS);
        System.out.printf("%s\n", CMD_QUERY_REACTIVE);
        System.out.printf("%s [size]\n", CMD_BULK_WRITE);
        System.out.printf("%s [size]\n", CMD_BULK_WRITE_REACTIVE);
        System.out.printf("%s [term]\n", CMD_SEARCH);
        System.out.println(CMD_QUIT);
    }

}
