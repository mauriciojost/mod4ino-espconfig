// https://jenkins.io/doc/book/pipeline/jenkinsfile/
// Scripted pipeline (not declarative)
// Use the snippet generator for more help: https://jenkins.martinenhome.com/job/botino-arduino/pipeline-syntax/
pipeline {
  options {
    buildDiscarder(logRotator(numToKeepStr: '10'))
  }
  agent {
    docker { 
      image 'mauriciojost/arduino-ci:platformio-3.5.3-0.2.0' 
    }
  }
  stages {
    stage('Build') {
      steps {
	echo "My branch is: ${env.BRANCH_NAME}"
	sh 'export GIT_COMMITTER_NAME=mjost && export GIT_COMMITTER_EMAIL=mauriciojost@gmail.com && set && ./pull_dependencies -p -l'
      }
    }
    stage('Test') {
      steps {
	echo "My branch is: ${env.BRANCH_NAME}"
        sh './launch_tests'
      }
    }
  }
  post {  
    failure {  
      emailext body: "<b>[JENKINS] Failure</b>Project: ${env.JOB_NAME} <br>Build Number: ${env.BUILD_NUMBER} <br> Build URL: ${env.BUILD_URL}", from: '', mimeType: 'text/html', replyTo: '', subject: "ERROR CI: ${env.JOB_NAME}", to: "mauriciojostx@gmail.com", attachLog: true, compressLog: false;
    }  
    success {  
      emailext body: "<b>[JENKINS] Success</b>Project: ${env.JOB_NAME} <br>Build Number: ${env.BUILD_NUMBER} <br> Build URL: ${env.BUILD_URL}", from: '', mimeType: 'text/html', replyTo: '', subject: "SUCCESS CI: ${env.JOB_NAME}", to: "mauriciojostx@gmail.com", attachLog: false, compressLog: false;
      cobertura autoUpdateHealth: false, autoUpdateStability: false, coberturaReportFile: 'coverage.xml', conditionalCoverageTargets: '70, 0, 0', enableNewApi: true, failUnhealthy: false, failUnstable: false, lineCoverageTargets: '80, 0, 0', maxNumberOfBuilds: 0, methodCoverageTargets: '80, 0, 0', onlyStable: false, sourceEncoding: 'ASCII', zoomCoverageChart: false
    }  
  }
}
