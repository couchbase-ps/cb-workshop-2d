FROM openjdk:11

WORKDIR /work
ADD . .
RUN ./mvnw compile assembly:single

FROM openjdk:11

WORKDIR /work
COPY --from=0 /work/target/CbDevWorkshop-*-jar-with-dependencies.jar CbDevWorkshop.jar

ENTRYPOINT java -cp CbDevWorkshop.jar -Dcbworkshop.clusteraddress=couchbase -Dcbworkshop.user=admin -Dcbworkshop.password=password -Dcbworkshop.bucket=travel-sample com.cbworkshop.MainLab
