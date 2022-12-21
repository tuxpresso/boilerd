pipeline {
    agent {
        dockerfile {
            args '-v ${WORKSPACE}:/ws'
        }
    }
    stages {
        stage('Configure') {
            steps {
                sh 'make -C /ws clean'
            }
        }
        stage('Build') {
            steps {
                sh 'make -C /ws'
            }
        }
    }
}
