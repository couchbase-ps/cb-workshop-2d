# Couchbase Development Workshop - 2 days for Amadeus

Couchbase is the most powerful NoSQL data platform.

This course is designed to give an introduction into Couchbase's main use cases, teach the Couchbase Server's administration basics and it's architecture. The main focus of this 2 day workshop is to learn how to develop with Couchbase's standard development kits. You will learn how to manage connections, how to work with documents and how to model your data best for Couchbase.

This is a combined C/C++ & Java workshop. 

## Requirements

Each training computer should have at least the following HW configuration.

### Hardware

  * 4 CPU cores >=2GHz
  * 8 GB RAM
  * 50 GB free disk space

The following connectivity is expected:

### Internet access

The following software needs to be installed on the attendee's computer:

  * Docker
  * Firefox or Chrome or Safari

The attendee should have all required permissions to install and run Docker.

### Room Facilities

The room should provide a video-projector (HDMI, DVI or VGA) & a whiteboard with eraser & pens.

### Docker Steup

You can run Couchbase inside a Docker container with a 1 node pre-configured cluster using the following command:

```bash
docker run -d -p 8091-8094:8091-8094 -p 11210:11210 --name couchbase couchbase/server-sandbox:6.6.5
```

Or run the C/Java labs by using the following command after changing dir to `java` or `cpp` directory:

```bash
docker-compose up --build -d
```

## Slides & Labs

* Day 1.
  * Slides: [Workshop-Day1-0-Agenda.pdf](slides/Day1/Workshop-Day1-0-Agenda.pdf)
  * Slides: [Workshop-Day1-1-Introduction.pdf](slides/Day1/Workshop-Day1-1-Introduction.pdf)
  * Slides: [Workshop-Day1-2-Architecture.pdf](slides/Day1/Workshop-Day1-2-Architecture.pdf)
  * Slides: [Workshop-Day1-3-Security.pdf](slides/Day1/Workshop-Day1-3-Security.pdf)
  * Slides: [Workshop-Day1-4-HAandDR.pdf](slides/Day1/Workshop-Day1-4-HAandDR.pdf)
  * Labs: [Workshop-Day1-5-Labs](slides/Day1/Workshop-Day1-5-Labs)

* Day 2.
  * Slides: [Workshop-Day2-1-Data Modeling.pdf](slides/Day2/Workshop-Day2-1-Data Modeling.pdf)
  * Slides: [Workshop-Day2-2-Whats new in Couchbase 6.6.pdf](slides/Day2/Workshop-Day2-2-Whats new in Couchbase 6.6.pdf)
  * Slides: [Workshop-Day2-3-N1QL.pdf](slides/Day2/Workshop-Day2-3-N1QL.pdf)
  * Slides: [Workshop-Day2-4-FTS.pdf](slides/Day2/Workshop-Day2-4-FTS.pdf)
  * Slides: [Workshop-Day2-5-N1QL-Labs.pdf](slides/Day2/Workshop-Day2-5-N1QL-Labs.pdf)
  * Labs C: [Workshop-Day2-6-CCPP-Labs.pdf](slides/Day2/Workshop-Day2-6-C-Labs.pdf)
  * Labs Java: [Workshop-Day2-6-Java-Labs.pdf](slides/Day2/Workshop-Day2-6-Java-Labs.pdf)

## Resources

* [Intro To Reactor Core](https://www.baeldung.com/reactor-core)
* [Couchbase in Docker](https://docs.couchbase.com/server/current/install/getting-started-docker.html)
