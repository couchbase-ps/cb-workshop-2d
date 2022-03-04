package com.cbworkshop;

import com.couchbase.client.core.env.TimeoutConfig;
import com.couchbase.client.core.error.DocumentNotFoundException;
import com.couchbase.client.java.Cluster;
import com.couchbase.client.java.Collection;
import com.couchbase.client.java.env.ClusterEnvironment;
import com.couchbase.client.java.json.JsonObject;
import com.couchbase.client.java.kv.GetResult;
import com.couchbase.client.java.kv.MutateInSpec;
import com.couchbase.client.java.query.*;
import com.couchbase.client.java.search.SearchQuery;
import com.couchbase.client.java.search.queries.MatchQuery;
import com.couchbase.client.java.search.result.SearchResult;
import com.couchbase.client.java.search.result.SearchRow;
import reactor.core.publisher.Flux;

import java.time.Duration;
import java.util.*;
import java.util.stream.IntStream;

import static com.couchbase.client.java.ClusterOptions.clusterOptions;
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
    public static final String CMD_BULK_WRITE_SYNC = "bulkwritesync";
    public static final String CMD_SEARCH = "search";

    private static Cluster cluster = null;
    private static Collection collection = null;

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

        ClusterEnvironment env = ClusterEnvironment.builder()
                .timeoutConfig(TimeoutConfig
                        .connectTimeout(Duration.ofSeconds(15))
                        .kvTimeout(Duration.ofSeconds(15)))
                .build();

        cluster = Cluster.connect(clusterAddress,
                clusterOptions(user, password)
                        .environment(env));
        collection = cluster.bucket(bucketName).defaultCollection();
    }

    private static void process(String cmdLn) {
        String words[] = cmdLn.split(" ");

        switch (words[0].toLowerCase()) {
            case CMD_QUIT:
                System.out.println("bye!");
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
                query(words);
                break;
            case CMD_QUERY_REACTIVE:
                queryReactive(words);
                break;
            case CMD_QUERY_AIRPORTS:
                queryAirports(words);
                break;
            case CMD_BULK_WRITE:
                bulkWrite(words);
                break;
            case CMD_BULK_WRITE_SYNC:
                bulkWriteSync(words);
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
        String key = "msg::" + words[1];
        String from = words[2];
        String to = words[3];
        JsonObject json = JsonObject.create()
                .put("timestamp", System.currentTimeMillis())
                .put("from", from)
                .put("to", to)
                .put("type", "msg");
        collection.insert(key, json);
        //collection.upsert(key, json);
        System.out.println("Document created with key: " + key);
    }

    private static void read(String[] words) {
        String key = words[1];
        GetResult result = null;
        try {
            result = collection.get(key);
            System.out.println(result.contentAsObject().toString());
        } catch (DocumentNotFoundException e) {
            System.out.printf("Document with key: %s not found%n", key);
        }
    }

    private static void update(String[] words) {
        String key = "airline_" + words[1];
        GetResult result = collection.get(key);
        JsonObject doc = result.contentAsObject();
        String name = doc.getString("name");
        doc.put("name", name.toUpperCase());
        collection.replace(key, doc);
    }

    private static void subdoc(String[] words) {
        String key = "msg::" + words[1];
        collection
                .mutateIn(key, List.of(
                        MutateInSpec.replace("from", "Administrator"),
                        MutateInSpec.insert("reviewed", System.currentTimeMillis())
                ));
    }

    private static void delete(String[] words) {
        String key = "msg::" + words[1];
        try {
            collection.remove(key);
        } catch (DocumentNotFoundException e) {
            System.out.printf("Document with key: %s not found%n", key);
        }
    }

    private static void query(String[] words) {
        QueryResult queryResult = cluster.query("SELECT * FROM `travel-sample` LIMIT 10");
        for (JsonObject row : queryResult.rowsAsObject()) {
            System.out.println(row.toString());
        }
    }

    private static void queryReactive(String[] words) {

        cluster.reactive()
                .query("SELECT * FROM `travel-sample` LIMIT 5")
                .flatMapMany(ReactiveQueryResult::rowsAsObject)
                .subscribe(row -> System.out.println(row.toString()),
                        e -> System.err.println("N1QL Error/Warning: " + e));

    }

    private static void queryAirports(String[] words) {
        String sourceairport = words[1];
        String destinationairport = words[2];
        String queryStr = "SELECT a.name FROM `travel-sample` r JOIN `travel-sample` a ON KEYS r.airlineid " +
                "WHERE r.type='route' AND r.sourceairport=$src AND r.destinationairport=$dst";

        JsonObject params = JsonObject.create()
                .put("src", sourceairport)
                .put("dst", destinationairport);

        QueryResult queryResult = cluster.query(queryStr, queryOptions().parameters(params));
        for (JsonObject row : queryResult.rowsAsObject()) {
            System.out.println(row.toString());
        }
    }

    private static void bulkWrite(String[] words) {
        int size = Integer.parseInt(words[1]);

        System.out.println("Deleting messages ...");
        cluster.query("DELETE FROM `travel-sample` WHERE type='msg'");

        System.out.printf("Writing %d messages%n", size);
        long ini = System.currentTimeMillis();
        Flux.range(0, size)
                .flatMap(i -> collection.reactive().insert("msg::" + i, JsonObject.create()
                        .put("timestamp", System.currentTimeMillis())
                        .put("from", "me")
                        .put("to", "you")
                        .put("type", "msg")))
                .blockLast();
        System.out.printf("Time elapsed %d ms%n", System.currentTimeMillis() - ini);
    }

    private static void bulkWriteSync(String[] words) {

        int size = Integer.parseInt(words[1]);

        System.out.println("Deleting messages ...");
        cluster.query("DELETE FROM `travel-sample` WHERE type='msg'");

        System.out.printf("Writing %d messages%n", size);
        long ini = System.currentTimeMillis();
        IntStream.range(0, size)
                .forEach(i -> collection.insert("msg::" + i, JsonObject.create()
                        .put("timestamp", System.currentTimeMillis())
                        .put("from", "me")
                        .put("to", "you")
                        .put("type", "msg")));

        System.out.printf("Time elapsed %d ms%n", System.currentTimeMillis() - ini);
    }

    private static void search(String[] words) {
        String term = words[1];
        MatchQuery fts = SearchQuery.match(term);
        SearchResult result = cluster.searchQuery("sidx_hotel_desc", fts);
        for (SearchRow row : result.rows()) {
            System.out.println(row);
        }
    }


    private static void welcome() {
        System.out.println("Welcome to CouchbaseJavaWorkshop!");
    }

    private static void usage() {
        System.out.println("Usage options: \n\n" + CMD_CREATE + " [key from to] \n" + CMD_READ + " [key] \n"
                + CMD_UPDATE + " [airline_key] \n" + CMD_SUBDOC + " [msg_key] \n" + CMD_DELETE + " [msg_key] \n"
                + CMD_QUERY + " \n" + CMD_QUERY_AIRPORTS + " [sourceairport destinationairport] \n"
                + CMD_QUERY_REACTIVE + " \n" + CMD_BULK_WRITE + " [size] \n" + CMD_BULK_WRITE_SYNC + " [size] \n"
                + CMD_SEARCH + " [term] \n" + CMD_QUIT);
    }

}
