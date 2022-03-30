# Couchbase C/C++ Workshop

This page contains details how to use the workshop material.

## Exercises

### Day 2: Using the Couchbase C Client Library

The starting point for the day 2 execises is the 'try-cb-lcb-labs' application. This is basically an application skeleton which has:

* All the header files
* An empty implementation of the required methods
* Test/demo cases

It will implement a REST service using the Kore.io framework.

The application 'try-cb-lcb' then contains all the exercise solutions.

| #               | Title                                  | Content                                      | 
| --------------- | -------------------------------------- | -------------------------------------------- |
| 1 | Setup Project Environment | Download the Couchbase C SDK |
| | | Install the required libraries and frameworks |
| | | Build and run the shell application |
| 2 | Managing Connections | Implement the connection to Couchbase in the `try-cb-lcb.c` source file. |
| | | Create and configure the connection in the `kore_worker_configure()` method. |
| | | Cleanup and destroy the connection instance in the `destroy_cb_instance()` method. |
| 3 | Create a Document | Create a user account document to register a user. |
| | | Edit the `api-user-auth.c` source file. |
| | | Create the user registration document in the `insert_user()` method, inserting the document into Couchbase. |
| 4 | Read a SubDocument | Edit the `get_user_password()` method. Read a property from the user account document using the subdoc methods. |
| | | Retrieve the `password` property using the user name as the document ID. |
| 5 | Upsert a Document | Edit the `api-user-flights.c` source file. |
| | | Modify the `upsert_new_flight()` method to upsert a user flights document, containing a list of flights the user has purchased. |
| 6 | Modify an existing document | Modify the `add_user_booking()` method, using the SubDoc API to append a flight to the list of flights the user has purchased. |
| 7 | Get a Document | Modify the `get_flight_booking()` method to retrieve the flight information document. |
| 8 | Query Via N1QL | Edit the `api-flight-paths.c` source document. |
| | | Edit the `tcblcb_api_fpaths()` method to implement two queries. |
| | | The first query will use **_positional_** parameters. |
| | | The second query will use **_named_** parameters. |
| 9 | Run the Front-End Application | Download and run the [try-cb-frontend-v2](https://github.com/couchbaselabs/try-cb-frontend-v2) frontend application |

